#include "handler_submit.hpp"
#include "db.hpp"
#include "common.hpp"
#include "rate_limiter.hpp"
#include <hiredis/hiredis.h>
#include <mutex>

static std::mutex redis_mutex;
static redisContext* redis_ctx = nullptr;

void init_redis(const std::string& host, int port) {
    redis_ctx = redisConnect(host.c_str(), port);
    if (!redis_ctx || redis_ctx->err) {
        throw std::runtime_error("Redis connect failed: " +
            std::string(redis_ctx ? redis_ctx->errstr : "unknown"));
    }
}

static void push_submit_queue(const json& job) {
    if (!redis_ctx) throw std::runtime_error("Redis not available");
    std::lock_guard<std::mutex> lock(redis_mutex);
    std::string payload = job.dump();
    redisReply* reply = (redisReply*)redisCommand(redis_ctx,
        "LPUSH submit_queue %b", payload.c_str(), payload.size());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        throw std::runtime_error("Redis LPUSH failed");
    }
    freeReplyObject(reply);
}

void handle_submit(const httplib::Request& req, httplib::Response& res) {
    auto user = auth_middleware(extract_session(req));
    if (!user) return error_response(res, 401, "UNAUTHORIZED", "请先登录");

    json body;
    try {
        body = json::parse(req.body);
    } catch (...) {
        return error_response(res, 400, "BAD_REQUEST", "请求格式错误");
    }

    if (!body.contains("problem_id") || !body.contains("code")) {
        return error_response(res, 400, "BAD_REQUEST", "缺少必要参数");
    }

    int problem_id = body["problem_id"].get<int>();
    std::string code = body["code"].get<std::string>();

    if (code.size() > 65536) {
        return error_response(res, 400, "BAD_REQUEST", "代码长度不能超过64KB");
    }

    auto db = DbPool::instance().guard();

    MYSQL_RES* check = db_query(db.get(),
        "SELECT id FROM problems WHERE id=" + std::to_string(problem_id));
    if (mysql_num_rows(check) == 0) {
        mysql_free_result(check);
        return error_response(res, 404, "NOT_FOUND", "题目不存在");
    }
    mysql_free_result(check);

    MYSQL_RES* pending = db_query(db.get(),
        "SELECT id FROM submissions WHERE user_id=" + std::to_string(user->id) +
        " AND problem_id=" + std::to_string(problem_id) +
        " AND status IN ('pending','compiling','running')");
    if (mysql_num_rows(pending) > 0) {
        mysql_free_result(pending);
        return error_response(res, 409, "CONFLICT", "已有待评测提交，请等待完成");
    }
    mysql_free_result(pending);

    if (!RateLimiter::instance().try_consume(user->id)) {
        return error_response(res, 429, "RATE_LIMITED", "提交过于频繁，请稍后再试");
    }

    std::string sql = "INSERT INTO submissions (user_id, problem_id, code, status) VALUES ("
        + std::to_string(user->id) + ", "
        + std::to_string(problem_id) + ", '"
        + db_escape(db.get(), code) + "', 'pending')";
    db_execute(db.get(), sql);
    long long submission_id = db_last_insert_id(db.get());

    MYSQL_RES* prob = db_query(db.get(),
        "SELECT time_limit_ms, memory_limit_kb FROM problems WHERE id="
        + std::to_string(problem_id));
    MYSQL_ROW prob_row = mysql_fetch_row(prob);
    int time_limit = prob_row[0] ? std::stoi(prob_row[0]) : 2000;
    int memory_limit = prob_row[1] ? std::stoi(prob_row[1]) : 262144;
    mysql_free_result(prob);

    json job;
    job["submission_id"] = submission_id;
    job["code"] = code;
    job["problem_id"] = problem_id;
    job["time_limit"] = time_limit;
    job["memory_limit"] = memory_limit;

    try {
        push_submit_queue(job);
    } catch (const std::exception& e) {
        RateLimiter::instance().refund(user->id);
        return error_response(res, 500, "INTERNAL_ERROR", "提交失败，请重试");
    }

    json resp;
    resp["data"] = json::object();
    resp["data"]["submission_id"] = submission_id;
    json_response(res, resp);
}

void handle_get_submission(const httplib::Request& req, httplib::Response& res) {
    auto user = auth_middleware(extract_session(req));
    if (!user) return error_response(res, 401, "UNAUTHORIZED", "请先登录");

    int submission_id = std::stoi(req.matches[1]);
    auto db = DbPool::instance().guard();

    std::string sql = "SELECT id, user_id, problem_id, code, status, failed_case, "
        "time_used_ms, memory_used_kb, passed_cases, total_cases, compile_error, "
        "detail_json, submitted_at, judged_at FROM submissions WHERE id="
        + std::to_string(submission_id);
    MYSQL_RES* result = db_query(db.get(), sql);

    if (mysql_num_rows(result) == 0) {
        mysql_free_result(result);
        return error_response(res, 404, "NOT_FOUND", "提交不存在");
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    json sub;
    sub["id"] = row[0] ? std::stoi(row[0]) : 0;
    sub["user_id"] = row[1] ? std::stoi(row[1]) : 0;
    sub["problem_id"] = row[2] ? std::stoi(row[2]) : 0;
    sub["code"] = row[3] ? row[3] : "";
    sub["status"] = row[4] ? row[4] : "pending";
    sub["failed_case"] = row[5] ? json(std::stoi(row[5])) : json(nullptr);
    sub["time_used_ms"] = row[6] ? json(std::stoi(row[6])) : json(nullptr);
    sub["memory_used_kb"] = row[7] ? json(std::stoi(row[7])) : json(nullptr);
    sub["passed_cases"] = row[8] ? std::stoi(row[8]) : 0;
    sub["total_cases"] = row[9] ? std::stoi(row[9]) : 0;
    sub["compile_error"] = row[10] ? json(row[10]) : json(nullptr);
    sub["submitted_at"] = row[12] ? row[12] : "";
    sub["judged_at"] = row[13] ? json(row[13]) : json(nullptr);

    if (row[11] && row[11][0] && std::string(row[11]) != "null") {
        try {
            sub["detail"] = json::parse(row[11]);
        } catch (...) {}
    }

    mysql_free_result(result);

    json resp;
    resp["data"] = sub;
    json_response(res, resp);
}

void handle_get_submissions(const httplib::Request& req, httplib::Response& res) {
    auto user = auth_middleware(extract_session(req));
    if (!user) return error_response(res, 401, "UNAUTHORIZED", "请先登录");

    int page = req.has_param("page") ? std::stoi(req.get_param_value("page")) : 1;
    int page_size = req.has_param("page_size") ? std::stoi(req.get_param_value("page_size")) : 20;
    if (page < 1) page = 1;
    if (page_size < 1 || page_size > 100) page_size = 20;

    auto db = DbPool::instance().guard();

    std::string count_sql = "SELECT COUNT(*) FROM submissions WHERE user_id="
        + std::to_string(user->id);
    MYSQL_RES* count_res = db_query(db.get(), count_sql);
    MYSQL_ROW count_row = mysql_fetch_row(count_res);
    int total = count_row[0] ? std::stoi(count_row[0]) : 0;
    mysql_free_result(count_res);

    int offset = (page - 1) * page_size;
    std::string sql = "SELECT id, problem_id, status, passed_cases, total_cases, "
        "time_used_ms, memory_used_kb, submitted_at, judged_at "
        "FROM submissions WHERE user_id=" + std::to_string(user->id) +
        " ORDER BY id DESC LIMIT " + std::to_string(page_size) +
        " OFFSET " + std::to_string(offset);
    MYSQL_RES* result = db_query(db.get(), sql);

    json items = json::array();
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        json sub;
        sub["id"] = row[0] ? std::stoi(row[0]) : 0;
        sub["problem_id"] = row[1] ? std::stoi(row[1]) : 0;
        sub["status"] = row[2] ? row[2] : "";
        sub["passed_cases"] = row[3] ? std::stoi(row[3]) : 0;
        sub["total_cases"] = row[4] ? std::stoi(row[4]) : 0;
        sub["time_used_ms"] = row[5] ? json(std::stoi(row[5])) : json(nullptr);
        sub["memory_used_kb"] = row[6] ? json(std::stoi(row[6])) : json(nullptr);
        sub["submitted_at"] = row[7] ? row[7] : "";
        sub["judged_at"] = row[8] ? json(row[8]) : json(nullptr);
        items.push_back(sub);
    }
    mysql_free_result(result);

    json resp;
    resp["data"] = json::object();
    resp["data"]["items"] = items;
    resp["data"]["total"] = total;
    resp["data"]["page"] = page;
    resp["data"]["page_size"] = page_size;
    json_response(res, resp);
}

void handle_report_result(const httplib::Request& req, httplib::Response& res) {
    auto token_it = req.headers.find("Authorization");
    const char* expected_token = std::getenv("JUDGE_API_TOKEN");
    std::string expected = "Bearer " + (expected_token ? std::string(expected_token) : "");
    if (token_it == req.headers.end() || token_it->second != expected) {
        return error_response(res, 401, "UNAUTHORIZED", "评测机认证失败");
    }

    int submission_id = std::stoi(req.matches[1]);
    json body;
    try {
        body = json::parse(req.body);
    } catch (...) {
        return error_response(res, 400, "BAD_REQUEST", "请求格式错误");
    }

    std::string status = body.value("status", "system_error");
    int passed_cases = body.value("passed_cases", 0);
    int total_cases = body.value("total_cases", 0);
    int time_used = body.value("time_used_ms", 0);
    int memory_used = body.value("memory_used_kb", 0);
    int failed_case = body.value("failed_case", -1);
    std::string compile_error = body.value("compile_error", "");

    json detail;
    if (body.contains("input_data")) detail["input_data"] = body["input_data"];
    if (body.contains("expected_output")) detail["expected_output"] = body["expected_output"];
    if (body.contains("actual_output")) detail["actual_output"] = body["actual_output"];

    auto db = DbPool::instance().guard();

    std::string sql = "UPDATE submissions SET status='" + db_escape(db.get(), status)
        + "', passed_cases=" + std::to_string(passed_cases)
        + ", total_cases=" + std::to_string(total_cases)
        + ", time_used_ms=" + std::to_string(time_used)
        + ", memory_used_kb=" + std::to_string(memory_used)
        + ", compile_error='" + db_escape(db.get(), compile_error) + "'"
        + ", detail_json='" + db_escape(db.get(), detail.dump()) + "'"
        + ", judged_at=NOW()";

    if (failed_case >= 0 && (status == "wrong_answer" || status == "time_limit_exceeded")) {
        sql += ", failed_case=" + std::to_string(failed_case);
    }

    sql += " WHERE id=" + std::to_string(submission_id);
    db_execute(db.get(), sql);

    if (status == "compilation_error") {
        MYSQL_RES* user_res = db_query(db.get(),
            "SELECT user_id FROM submissions WHERE id=" + std::to_string(submission_id));
        if (mysql_num_rows(user_res) > 0) {
            MYSQL_ROW user_row = mysql_fetch_row(user_res);
            int uid = std::stoi(user_row[0]);
            RateLimiter::instance().refund(uid);
        }
        mysql_free_result(user_res);
    }

    json resp;
    resp["data"] = json::object();
    resp["data"]["ok"] = true;
    json_response(res, resp);
}
