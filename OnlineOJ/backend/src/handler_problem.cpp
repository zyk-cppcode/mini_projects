#include "handler_problem.hpp"
#include "db.hpp"
#include "common.hpp"

void handle_get_problems(const httplib::Request& req, httplib::Response& res) {
    auto user = auth_middleware(extract_session(req));
    if (!user) return error_response(res, 401, "UNAUTHORIZED", "请先登录");

    int page = req.has_param("page") ? std::stoi(req.get_param_value("page")) : 1;
    int page_size = req.has_param("page_size") ? std::stoi(req.get_param_value("page_size")) : 20;
    if (page < 1) page = 1;
    if (page_size < 1 || page_size > 100) page_size = 20;

    std::string difficulty = req.has_param("difficulty") ? req.get_param_value("difficulty") : "";
    std::string tag = req.has_param("tag") ? req.get_param_value("tag") : "";

    auto db = DbPool::instance().guard();

    std::string where;
    if (!difficulty.empty()) where += " AND p.difficulty='" + db_escape(db.get(), difficulty) + "'";
    if (!tag.empty()) where += " AND pt_tag.name='" + db_escape(db.get(), tag) + "'";

    std::string count_sql =
        "SELECT COUNT(DISTINCT p.id) FROM problems p "
        "LEFT JOIN problem_tags pt ON p.id = pt.problem_id "
        "LEFT JOIN tags pt_tag ON pt.tag_id = pt_tag.id WHERE 1=1" + where;
    MYSQL_RES* count_res = db_query(db.get(), count_sql);
    MYSQL_ROW count_row = mysql_fetch_row(count_res);
    int total = count_row[0] ? std::stoi(count_row[0]) : 0;
    mysql_free_result(count_res);

    int offset = (page - 1) * page_size;
    std::string sql =
        "SELECT DISTINCT p.id, p.title, p.description, p.language, p.difficulty, p.time_limit_ms, "
        "p.memory_limit_kb, p.author_id, p.created_at, p.updated_at "
        "FROM problems p "
        "LEFT JOIN problem_tags pt ON p.id = pt.problem_id "
        "LEFT JOIN tags pt_tag ON pt.tag_id = pt_tag.id WHERE 1=1" + where +
        " ORDER BY p.id DESC LIMIT " + std::to_string(page_size) +
        " OFFSET " + std::to_string(offset);

    MYSQL_RES* result = db_query(db.get(), sql);
    json items = json::array();

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        int problem_id = std::stoi(row[0]);

        std::string tag_sql = "SELECT t.name FROM tags t "
            "JOIN problem_tags pt ON t.id = pt.tag_id WHERE pt.problem_id="
            + std::to_string(problem_id);
        MYSQL_RES* tag_res = db_query(db.get(), tag_sql);
        json tags = json::array();
        MYSQL_ROW tag_row;
        while ((tag_row = mysql_fetch_row(tag_res))) {
            tags.push_back(tag_row[0] ? tag_row[0] : "");
        }
        mysql_free_result(tag_res);

        json problem;
        problem["id"] = problem_id;
        problem["title"] = row[1] ? row[1] : "";
        problem["description"] = row[2] ? row[2] : "";
        problem["language"] = row[3] ? row[3] : "cpp";
        problem["difficulty"] = row[4] ? row[4] : "easy";
        problem["time_limit_ms"] = row[5] ? std::stoi(row[5]) : 2000;
        problem["memory_limit_kb"] = row[6] ? std::stoi(row[6]) : 262144;
        problem["author_id"] = row[7] ? std::stoi(row[7]) : 0;
        problem["created_at"] = row[8] ? row[8] : "";
        problem["updated_at"] = row[9] ? row[9] : "";
        problem["tags"] = tags;
        items.push_back(problem);
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

void handle_get_problem(const httplib::Request& req, httplib::Response& res) {
    auto user = auth_middleware(extract_session(req));
    if (!user) return error_response(res, 401, "UNAUTHORIZED", "请先登录");

    int problem_id = std::stoi(req.matches[1]);

    auto db = DbPool::instance().guard();

    std::string sql = "SELECT id, title, description, language, difficulty, code_template, "
        "time_limit_ms, memory_limit_kb, author_id, created_at, updated_at "
        "FROM problems WHERE id=" + std::to_string(problem_id);
    MYSQL_RES* result = db_query(db.get(), sql);

    if (mysql_num_rows(result) == 0) {
        mysql_free_result(result);
        return error_response(res, 404, "NOT_FOUND", "题目不存在");
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    json problem;
    problem["id"] = std::stoi(row[0]);
    problem["title"] = row[1] ? row[1] : "";
    problem["description"] = row[2] ? row[2] : "";
    problem["language"] = row[3] ? row[3] : "cpp";
    problem["difficulty"] = row[4] ? row[4] : "easy";
    problem["code_template"] = row[5] ? json(row[5]) : json(nullptr);
    problem["time_limit_ms"] = row[6] ? std::stoi(row[6]) : 2000;
    problem["memory_limit_kb"] = row[7] ? std::stoi(row[7]) : 262144;
    problem["author_id"] = row[8] ? std::stoi(row[8]) : 0;
    problem["created_at"] = row[9] ? row[9] : "";
    problem["updated_at"] = row[10] ? row[10] : "";

    std::string tag_sql = "SELECT t.name FROM tags t "
        "JOIN problem_tags pt ON t.id = pt.tag_id WHERE pt.problem_id="
        + std::to_string(problem_id);
    MYSQL_RES* tag_res = db_query(db.get(), tag_sql);
    json tags = json::array();
    MYSQL_ROW tag_row;
    while ((tag_row = mysql_fetch_row(tag_res))) {
        tags.push_back(tag_row[0] ? tag_row[0] : "");
    }
    mysql_free_result(tag_res);
    problem["tags"] = tags;

    mysql_free_result(result);

    json resp;
    resp["data"] = problem;
    json_response(res, resp);
}

void handle_get_testdata(const httplib::Request& req, httplib::Response& res) {
    auto token_it = req.headers.find("Authorization");
    const char* env_token = std::getenv("JUDGE_API_TOKEN");
    std::string expected = "Bearer " + (env_token ? std::string(env_token) : "");
    if (token_it == req.headers.end() || token_it->second != expected) {
        return error_response(res, 401, "UNAUTHORIZED", "评测机认证失败");
    }

    int problem_id = std::stoi(req.matches[1]);
    auto db = DbPool::instance().guard();

    std::string sql = "SELECT id, case_number, input_data, expected_output "
        "FROM test_cases WHERE problem_id=" + std::to_string(problem_id) +
        " ORDER BY case_number";
    MYSQL_RES* result = db_query(db.get(), sql);

    json testdata = json::array();
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        json tc;
        tc["id"] = row[0] ? std::stoi(row[0]) : 0;
        tc["case_number"] = row[1] ? std::stoi(row[1]) : 0;
        tc["input_data"] = row[2] ? row[2] : "";
        tc["expected_output"] = row[3] ? row[3] : "";
        testdata.push_back(tc);
    }
    mysql_free_result(result);

    json resp;
    resp["data"] = testdata;
    json_response(res, resp);
}
