#include "handler_admin.hpp"
#include "db.hpp"
#include "common.hpp"
#include <zip.h>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>
#include <map>

void require_admin_user(const httplib::Request& req, httplib::Response& res,
                        std::optional<User>& out) {
    out = auth_middleware(extract_session(req));
    if (!out) {
        error_response(res, 401, "UNAUTHORIZED", "请先登录");
        return;
    }
    if (out->role != "admin") {
        error_response(res, 403, "FORBIDDEN", "需要管理员权限");
        out.reset();
    }
}

void handle_admin_create_problem(const httplib::Request& req, httplib::Response& res) {
    std::optional<User> user;
    require_admin_user(req, res, user);
    if (!user) return;

    json body;
    try {
        body = json::parse(req.body);
    } catch (...) {
        return error_response(res, 400, "BAD_REQUEST", "请求格式错误");
    }

    std::string title = body.value("title", "");
    std::string description = body.value("description", "");
    std::string difficulty = body.value("difficulty", "easy");
    std::string language = body.value("language", "cpp");
    int time_limit = body.value("time_limit_ms", 2000);
    int memory_limit = body.value("memory_limit_kb", 262144);
    std::string code_template = body.value("code_template", "");

    if (title.empty()) return error_response(res, 400, "BAD_REQUEST", "标题不能为空");

    auto db = DbPool::instance().guard();

    std::string sql = "INSERT INTO problems (title, description, language, difficulty, "
        "time_limit_ms, memory_limit_kb, author_id";
    std::string values = " VALUES ('" + db_escape(db.get(), title) + "', '"
        + db_escape(db.get(), description) + "', '"
        + db_escape(db.get(), language) + "', '"
        + db_escape(db.get(), difficulty) + "', "
        + std::to_string(time_limit) + ", " + std::to_string(memory_limit)
        + ", " + std::to_string(user->id);

    if (!code_template.empty()) {
        sql += ", code_template";
        values += ", '" + db_escape(db.get(), code_template) + "'";
    }
    sql += ")" + values + ")";
    db_execute(db.get(), sql);
    long long problem_id = db_last_insert_id(db.get());

    if (body.contains("tags") && body["tags"].is_array()) {
        for (const auto& tag : body["tags"]) {
            std::string tag_name = tag.get<std::string>();
            MYSQL_RES* tag_res = db_query(db.get(),
                "SELECT id FROM tags WHERE name='" + db_escape(db.get(), tag_name) + "'");
            int tag_id;
            if (mysql_num_rows(tag_res) > 0) {
                MYSQL_ROW tr = mysql_fetch_row(tag_res);
                tag_id = std::stoi(tr[0]);
            } else {
                db_execute(db.get(),
                    "INSERT INTO tags (name) VALUES ('" + db_escape(db.get(), tag_name) + "')");
                tag_id = db_last_insert_id(db.get());
            }
            mysql_free_result(tag_res);
            db_execute(db.get(), "INSERT INTO problem_tags (problem_id, tag_id) VALUES ("
                + std::to_string(problem_id) + ", " + std::to_string(tag_id) + ")");
        }
    }

    json resp;
    resp["data"] = json::object();
    resp["data"]["id"] = problem_id;
    json_response(res, resp, 201);
}

void handle_admin_update_problem(const httplib::Request& req, httplib::Response& res) {
    std::optional<User> user;
    require_admin_user(req, res, user);
    if (!user) return;

    int problem_id = std::stoi(req.matches[1]);

    json body;
    try {
        body = json::parse(req.body);
    } catch (...) {
        return error_response(res, 400, "BAD_REQUEST", "请求格式错误");
    }

    auto db = DbPool::instance().guard();

    MYSQL_RES* check = db_query(db.get(),
        "SELECT id FROM problems WHERE id=" + std::to_string(problem_id));
    if (mysql_num_rows(check) == 0) {
        mysql_free_result(check);
        return error_response(res, 404, "NOT_FOUND", "题目不存在");
    }
    mysql_free_result(check);

    std::vector<std::string> updates;
    if (body.contains("title"))
        updates.push_back("title='" + db_escape(db.get(), body["title"].get<std::string>()) + "'");
    if (body.contains("description"))
        updates.push_back("description='" + db_escape(db.get(), body["description"].get<std::string>()) + "'");
    if (body.contains("difficulty"))
        updates.push_back("difficulty='" + db_escape(db.get(), body["difficulty"].get<std::string>()) + "'");
    if (body.contains("language"))
        updates.push_back("language='" + db_escape(db.get(), body["language"].get<std::string>()) + "'");
    if (body.contains("time_limit_ms"))
        updates.push_back("time_limit_ms=" + std::to_string(body["time_limit_ms"].get<int>()));
    if (body.contains("memory_limit_kb"))
        updates.push_back("memory_limit_kb=" + std::to_string(body["memory_limit_kb"].get<int>()));
    if (body.contains("code_template")) {
        std::string tmpl = body["code_template"].get<std::string>();
        updates.push_back("code_template='" + db_escape(db.get(), tmpl) + "'");
    }

    if (!updates.empty()) {
        std::string sql = "UPDATE problems SET ";
        for (size_t i = 0; i < updates.size(); i++) {
            if (i > 0) sql += ", ";
            sql += updates[i];
        }
        sql += " WHERE id=" + std::to_string(problem_id);
        db_execute(db.get(), sql);
    }

    if (body.contains("tags") && body["tags"].is_array()) {
        db_execute(db.get(), "DELETE FROM problem_tags WHERE problem_id=" + std::to_string(problem_id));
        for (const auto& tag : body["tags"]) {
            std::string tag_name = tag.get<std::string>();
            MYSQL_RES* tag_res = db_query(db.get(),
                "SELECT id FROM tags WHERE name='" + db_escape(db.get(), tag_name) + "'");
            int tag_id;
            if (mysql_num_rows(tag_res) > 0) {
                MYSQL_ROW tr = mysql_fetch_row(tag_res);
                tag_id = std::stoi(tr[0]);
            } else {
                db_execute(db.get(),
                    "INSERT INTO tags (name) VALUES ('" + db_escape(db.get(), tag_name) + "')");
                tag_id = db_last_insert_id(db.get());
            }
            mysql_free_result(tag_res);
            db_execute(db.get(), "INSERT INTO problem_tags (problem_id, tag_id) VALUES ("
                + std::to_string(problem_id) + ", " + std::to_string(tag_id) + ")");
        }
    }

    json resp;
    resp["data"] = json::object();
    resp["data"]["ok"] = true;
    json_response(res, resp);
}

void handle_admin_delete_problem(const httplib::Request& req, httplib::Response& res) {
    std::optional<User> user;
    require_admin_user(req, res, user);
    if (!user) return;

    int problem_id = std::stoi(req.matches[1]);
    auto db = DbPool::instance().guard();

    MYSQL_RES* check = db_query(db.get(),
        "SELECT id FROM problems WHERE id=" + std::to_string(problem_id));
    if (mysql_num_rows(check) == 0) {
        mysql_free_result(check);
        return error_response(res, 404, "NOT_FOUND", "题目不存在");
    }
    mysql_free_result(check);

    db_execute(db.get(), "DELETE FROM problem_tags WHERE problem_id=" + std::to_string(problem_id));
    db_execute(db.get(), "DELETE FROM test_cases WHERE problem_id=" + std::to_string(problem_id));
    db_execute(db.get(), "DELETE FROM problems WHERE id=" + std::to_string(problem_id));

    json resp;
    resp["data"] = json::object();
    resp["data"]["ok"] = true;
    json_response(res, resp);
}

void handle_admin_upload_testdata(const httplib::Request& req, httplib::Response& res) {
    std::optional<User> user;
    require_admin_user(req, res, user);
    if (!user) return;

    int problem_id = std::stoi(req.matches[1]);
    auto db = DbPool::instance().guard();

    MYSQL_RES* check = db_query(db.get(),
        "SELECT id FROM problems WHERE id=" + std::to_string(problem_id));
    if (mysql_num_rows(check) == 0) {
        mysql_free_result(check);
        return error_response(res, 404, "NOT_FOUND", "题目不存在");
    }
    mysql_free_result(check);

    std::string tmp_path = "/tmp/oj_testdata_" + std::to_string(problem_id) + ".zip";
    std::string extract_dir = "/tmp/oj_extract_" + std::to_string(problem_id);

    std::ofstream(tmp_path, std::ios::binary).write(req.body.data(), req.body.size());

    mkdir(extract_dir.c_str(), 0755);

    int err;
    zip_t* archive = zip_open(tmp_path.c_str(), ZIP_RDONLY, &err);
    if (!archive) {
        std::remove(tmp_path.c_str());
        return error_response(res, 400, "BAD_REQUEST", "ZIP 文件解析失败");
    }

    zip_int64_t num_entries = zip_get_num_entries(archive, 0);

    db_execute(db.get(), "DELETE FROM test_cases WHERE problem_id=" + std::to_string(problem_id));

    std::map<int, std::string> inputs, outputs;

    for (zip_int64_t i = 0; i < num_entries; i++) {
        const char* name = zip_get_name(archive, i, ZIP_FL_ENC_GUESS);
        if (!name) continue;

        std::string fname(name);
        zip_file_t* zf = zip_fopen_index(archive, i, ZIP_FL_ENC_GUESS);
        if (!zf) continue;

        std::string content;
        char buf[4096];
        zip_int64_t n;
        while ((n = zip_fread(zf, buf, sizeof(buf))) > 0) {
            content.append(buf, n);
        }
        zip_fclose(zf);

        if (fname.find(".in") != std::string::npos) {
            std::string num_str;
            for (char c : fname) {
                if (c >= '0' && c <= '9') num_str += c;
                else if (!num_str.empty()) break;
            }
            if (!num_str.empty()) inputs[std::stoi(num_str)] = content;
        } else if (fname.find(".out") != std::string::npos || fname.find(".ans") != std::string::npos) {
            std::string num_str;
            for (char c : fname) {
                if (c >= '0' && c <= '9') num_str += c;
                else if (!num_str.empty()) break;
            }
            if (!num_str.empty()) outputs[std::stoi(num_str)] = content;
        }
    }
    zip_close(archive);

    int case_count = 0;
    for (const auto& kv : inputs) {
        int num = kv.first;
        if (outputs.count(num) == 0) continue;
        case_count++;
        std::string sql = "INSERT INTO test_cases (problem_id, case_number, input_data, expected_output) VALUES ("
            + std::to_string(problem_id) + ", " + std::to_string(case_count) + ", '"
            + db_escape(db.get(), kv.second) + "', '"
            + db_escape(db.get(), outputs[num]) + "')";
        db_execute(db.get(), sql);
    }

    std::remove(tmp_path.c_str());
    std::string rm_cmd = "rm -rf " + extract_dir;
    system(rm_cmd.c_str());

    json resp;
    resp["data"] = json::object();
    resp["data"]["cases_count"] = case_count;
    json_response(res, resp);
}

void handle_admin_tags(const httplib::Request& req, httplib::Response& res) {
    std::optional<User> user;
    require_admin_user(req, res, user);
    if (!user) return;

    auto db = DbPool::instance().guard();

    if (req.method == "GET") {
        MYSQL_RES* result = db_query(db.get(), "SELECT id, name FROM tags ORDER BY id");
        json tags = json::array();
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            json tag;
            tag["id"] = row[0] ? std::stoi(row[0]) : 0;
            tag["name"] = row[1] ? row[1] : "";
            tags.push_back(tag);
        }
        mysql_free_result(result);
        json resp;
        resp["data"] = tags;
        json_response(res, resp);
    } else if (req.method == "POST") {
        json body;
        try { body = json::parse(req.body); } catch (...) {
            return error_response(res, 400, "BAD_REQUEST", "请求格式错误");
        }
        std::string name = body.value("name", "");
        if (name.empty()) return error_response(res, 400, "BAD_REQUEST", "标签名不能为空");
        db_execute(db.get(), "INSERT INTO tags (name) VALUES ('" + db_escape(db.get(), name) + "')");
        json resp;
        resp["data"] = json::object();
        resp["data"]["id"] = db_last_insert_id(db.get());
        json_response(res, resp, 201);
    } else if (req.method == "DELETE") {
        int tag_id = std::stoi(req.matches[1]);
        db_execute(db.get(), "DELETE FROM problem_tags WHERE tag_id=" + std::to_string(tag_id));
        db_execute(db.get(), "DELETE FROM tags WHERE id=" + std::to_string(tag_id));
        json resp;
        resp["data"] = json::object();
        resp["data"]["ok"] = true;
        json_response(res, resp);
    }
}
