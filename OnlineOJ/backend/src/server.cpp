#include "server.hpp"
#include "handler_problem.hpp"
#include "handler_submit.hpp"
#include "handler_admin.hpp"
#include "handler_leaderboard.hpp"
#include "auth.hpp"
#include "common.hpp"
#include <fstream>

void setup_server(httplib::Server& server) {
    server.set_mount_point("/", "../frontend/");

    server.Options(R"(/api/.*)", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type,Authorization,Cookie");
        res.set_header("Access-Control-Allow-Credentials", "true");
        res.status = 204;
    });

    // Auth
    server.Post("/api/register", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        try {
            json body = json::parse(req.body);
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            if (username.empty() || password.empty()) {
                resp["error"] = "用户名和密码不能为空";
                resp["code"] = "BAD_REQUEST";
                res.status = 400;
            } else if (username.size() > 50) {
                resp["error"] = "用户名最长50字符";
                resp["code"] = "BAD_REQUEST";
                res.status = 400;
            } else {
                User user = Auth::instance().register_user(username, password);
                resp["data"] = json::object();
                resp["data"]["id"] = user.id;
                resp["data"]["username"] = user.username;
            }
        } catch (const std::exception& e) {
            resp["error"] = e.what();
            resp["code"] = "BAD_REQUEST";
            res.status = 400;
        }
        res.set_content(resp.dump(), "application/json");
    });

    server.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        try {
            json body = json::parse(req.body);
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            std::string session_id = Auth::instance().login(username, password);
            resp["data"] = json::object();
            resp["data"]["ok"] = true;

            std::string cookie = "session=" + session_id
                + "; Path=/; HttpOnly; SameSite=Lax; Max-Age=86400";
            res.set_header("Set-Cookie", cookie);
        } catch (const std::exception& e) {
            resp["error"] = e.what();
            resp["code"] = "UNAUTHORIZED";
            res.status = 401;
        }
        res.set_content(resp.dump(), "application/json");
    });

    server.Post("/api/logout", [](const httplib::Request& req, httplib::Response& res) {
        std::string sid = extract_session(req);
        if (!sid.empty()) Auth::instance().logout(sid);
        json resp;
        resp["data"] = json::object();
        resp["data"]["ok"] = true;
        res.set_header("Set-Cookie", "session=; Path=/; Max-Age=0");
        res.set_content(resp.dump(), "application/json");
    });

    server.Get("/api/session", [](const httplib::Request& req, httplib::Response& res) {
        json resp;
        auto user = Auth::instance().get_user_by_session(extract_session(req));
        if (user) {
            resp["data"] = to_json_user(*user);
        } else {
            resp["data"] = nullptr;
        }
        res.set_content(resp.dump(), "application/json");
    });

    // Problems
    server.Get("/api/problems", handle_get_problems);
    server.Get(R"(/api/problems/(\d+))", handle_get_problem);
    server.Get(R"(/api/problems/(\d+)/testdata)", handle_get_testdata);

    // Submissions
    server.Post("/api/submit", handle_submit);
    server.Get("/api/submissions", handle_get_submissions);
    server.Get(R"(/api/submissions/(\d+))", handle_get_submission);
    server.Put(R"(/api/submissions/(\d+)/result)", handle_report_result);

    // Admin
    server.Post("/api/admin/problems", handle_admin_create_problem);
    server.Put(R"(/api/admin/problems/(\d+))", handle_admin_update_problem);
    server.Delete(R"(/api/admin/problems/(\d+))", handle_admin_delete_problem);
    server.Post(R"(/api/admin/problems/(\d+)/testdata)", handle_admin_upload_testdata);
    server.Get("/api/admin/tags", handle_admin_tags);
    server.Post("/api/admin/tags", handle_admin_tags);
    server.Delete(R"(/api/admin/tags/(\d+))", handle_admin_tags);

    // Leaderboard & Statistics
    server.Get("/api/leaderboard", handle_leaderboard);
    server.Get("/api/statistics", handle_statistics);
}
