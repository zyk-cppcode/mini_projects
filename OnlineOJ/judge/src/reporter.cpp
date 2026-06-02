#include "reporter.hpp"
#include <httplib.h>
#include <iostream>
#include <thread>
#include <chrono>

std::string g_backend_url;
std::string g_api_token;

void set_reporter_config(const std::string& backend_url, const std::string& api_token) {
    g_backend_url = backend_url;
    g_api_token = api_token;
}

std::vector<TestCase> fetch_testdata(int problem_id) {
    std::vector<TestCase> cases;
    httplib::Client cli(g_backend_url);
    cli.set_connection_timeout(10);
    cli.set_read_timeout(10);

    httplib::Headers headers = {{"Authorization", "Bearer " + g_api_token}};
    std::string path = "/api/problems/" + std::to_string(problem_id) + "/testdata";

    httplib::Result resp = cli.Get(path, headers);
    if (!resp || resp->status != 200) {
        std::cerr << "Failed to fetch testdata: "
                  << (resp ? std::to_string(resp->status) : "no response") << std::endl;
        return cases;
    }

    try {
        json j = json::parse(resp->body);
        if (j.contains("data") && j["data"].is_array()) {
            for (const auto& tc : j["data"]) {
                TestCase c;
                c.case_number = tc.value("case_number", 0);
                c.input_data = tc.value("input_data", "");
                c.expected_output = tc.value("expected_output", "");
                cases.push_back(c);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Parse testdata error: " << e.what() << std::endl;
    }
    return cases;
}

bool report_result(const JudgeResult& result) {
    json body;
    body["status"] = result.status;
    body["passed_cases"] = result.passed_cases;
    body["total_cases"] = result.total_cases;
    body["time_used_ms"] = result.time_used_ms;
    body["memory_used_kb"] = result.memory_used_kb;

    if (result.failed_case > 0) {
        body["failed_case"] = result.failed_case;
    }
    if (!result.compile_error.empty()) {
        body["compile_error"] = result.compile_error;
    }
    if (!result.input_data.empty()) {
        body["input_data"] = result.input_data;
        body["expected_output"] = result.expected_output;
        body["actual_output"] = result.actual_output;
    }

    std::string path = "/api/submissions/" + std::to_string(result.submission_id) + "/result";

    for (int retry = 0; retry < 3; retry++) {
        httplib::Client cli(g_backend_url);
        cli.set_connection_timeout(5);
        cli.set_read_timeout(5);

        httplib::Headers headers = {
            {"Authorization", "Bearer " + g_api_token},
            {"Content-Type", "application/json"}
        };

        httplib::Result resp = cli.Put(path, headers, body.dump(), "application/json");
        if (resp && resp->status == 200) {
            return true;
        }
        if (retry < 2) {
            std::this_thread::sleep_for(std::chrono::seconds(1 << retry));
        }
    }
    return false;
}
