#include "handler_leaderboard.hpp"
#include "db.hpp"
#include "common.hpp"

void handle_leaderboard(const httplib::Request& req, httplib::Response& res) {
    auto user = auth_middleware(extract_session(req));
    if (!user) return error_response(res, 401, "UNAUTHORIZED", "请先登录");

    std::string difficulty = req.has_param("difficulty") ? req.get_param_value("difficulty") : "";
    std::string tag = req.has_param("tag") ? req.get_param_value("tag") : "";

    auto db = DbPool::instance().guard();

    std::string join;
    if (!tag.empty()) {
        join = " JOIN problem_tags pt ON s.problem_id = pt.problem_id "
               "JOIN tags t ON pt.tag_id = t.id ";
    }

    std::string where = " WHERE s.status = 'accepted' ";
    if (!difficulty.empty()) {
        where += " AND p.difficulty='" + db_escape(db.get(), difficulty) + "'";
    }
    if (!tag.empty()) {
        where += " AND t.name='" + db_escape(db.get(), tag) + "'";
    }

    std::string sql =
        "SELECT u.id, u.username, "
        "COUNT(DISTINCT s.problem_id) AS solved, "
        "SUM(CASE WHEN s.status = 'accepted' THEN 1 ELSE 0 END) / NULLIF(COUNT(s.id), 0) AS rate "
        "FROM users u "
        "JOIN submissions s ON u.id = s.user_id "
        "JOIN problems p ON s.problem_id = p.id " + join + where +
        " GROUP BY u.id, u.username "
        "ORDER BY solved DESC, rate DESC "
        "LIMIT 100";

    MYSQL_RES* result = db_query(db.get(), sql);
    json items = json::array();
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        json entry;
        entry["user_id"] = row[0] ? std::stoi(row[0]) : 0;
        entry["username"] = row[1] ? row[1] : "";
        entry["solved_count"] = row[2] ? std::stoi(row[2]) : 0;
        entry["pass_rate"] = row[3] ? std::stod(row[3]) : 0.0;
        items.push_back(entry);
    }
    mysql_free_result(result);

    json resp;
    resp["data"] = items;
    json_response(res, resp);
}

void handle_statistics(const httplib::Request& req, httplib::Response& res) {
    auto user = auth_middleware(extract_session(req));
    if (!user) return error_response(res, 401, "UNAUTHORIZED", "请先登录");

    auto db = DbPool::instance().guard();

    std::string sql =
        "SELECT "
        "COUNT(DISTINCT CASE WHEN s.status = 'accepted' THEN s.problem_id END), "
        "COUNT(s.id), "
        "SUM(CASE WHEN s.status = 'accepted' THEN 1 ELSE 0 END) / NULLIF(COUNT(s.id), 0) "
        "FROM submissions s WHERE s.user_id=" + std::to_string(user->id);
    MYSQL_RES* result = db_query(db.get(), sql);
    MYSQL_ROW row = mysql_fetch_row(result);
    json stats;
    stats["solved_count"] = row[0] ? std::stoi(row[0]) : 0;
    stats["total_submissions"] = row[1] ? std::stoi(row[1]) : 0;
    stats["pass_rate"] = row[2] ? std::stod(row[2]) : 0.0;
    mysql_free_result(result);

    std::string diff_sql =
        "SELECT p.difficulty, COUNT(DISTINCT s.problem_id) "
        "FROM submissions s JOIN problems p ON s.problem_id = p.id "
        "WHERE s.user_id=" + std::to_string(user->id) + " AND s.status='accepted' "
        "GROUP BY p.difficulty";
    MYSQL_RES* diff_res = db_query(db.get(), diff_sql);
    int easy = 0, medium = 0, hard = 0;
    MYSQL_ROW dr;
    while ((dr = mysql_fetch_row(diff_res))) {
        std::string d = dr[0] ? dr[0] : "";
        int cnt = dr[1] ? std::stoi(dr[1]) : 0;
        if (d == "easy") easy = cnt;
        else if (d == "medium") medium = cnt;
        else if (d == "hard") hard = cnt;
    }
    mysql_free_result(diff_res);

    stats["easy_count"] = easy;
    stats["medium_count"] = medium;
    stats["hard_count"] = hard;

    json resp;
    resp["data"] = stats;
    json_response(res, resp);
}
