#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>

using json = nlohmann::json;

struct User {
    int id = 0;
    std::string username;
    std::string password_hash;
    std::string role = "user";
    std::string created_at;
};

struct Problem {
    int id = 0;
    std::string title;
    std::string description;
    std::string difficulty = "easy";
    std::optional<std::string> code_template;
    int time_limit_ms = 2000;
    int memory_limit_kb = 262144;
    int author_id = 0;
    std::string created_at;
    std::string updated_at;
    std::vector<std::string> tags;
};

struct TestCase {
    int id = 0;
    int problem_id = 0;
    int case_number = 0;
    std::string input_data;
    std::string expected_output;
    std::string created_at;
};

struct Submission {
    int id = 0;
    int user_id = 0;
    int problem_id = 0;
    std::string code;
    std::string status = "pending";
    std::optional<int> failed_case;
    std::optional<int> time_used_ms;
    std::optional<int> memory_used_kb;
    int passed_cases = 0;
    int total_cases = 0;
    std::optional<std::string> compile_error;
    std::optional<std::string> input_data;
    std::optional<std::string> expected_output;
    std::optional<std::string> actual_output;
    std::string submitted_at;
    std::optional<std::string> judged_at;
};

struct LeaderboardEntry {
    int user_id;
    std::string username;
    int solved_count;
    double pass_rate;
};

struct UserStats {
    int solved_count;
    int total_submissions;
    double pass_rate;
    int easy_count;
    int medium_count;
    int hard_count;
};

inline json to_json_user(const User& u) {
    return json{{"id", u.id}, {"username", u.username},
                {"role", u.role}, {"created_at", u.created_at}};
}

inline json to_json_problem(const Problem& p) {
    json j;
    j["id"] = p.id;
    j["title"] = p.title;
    j["description"] = p.description;
    j["difficulty"] = p.difficulty;
    if (p.code_template.has_value()) j["code_template"] = p.code_template.value();
    else j["code_template"] = nullptr;
    j["time_limit_ms"] = p.time_limit_ms;
    j["memory_limit_kb"] = p.memory_limit_kb;
    j["author_id"] = p.author_id;
    j["created_at"] = p.created_at;
    j["updated_at"] = p.updated_at;
    j["tags"] = p.tags;
    return j;
}

inline json to_json_testcase(const TestCase& tc) {
    return json{{"id", tc.id}, {"problem_id", tc.problem_id},
                {"case_number", tc.case_number}, {"input_data", tc.input_data},
                {"expected_output", tc.expected_output}, {"created_at", tc.created_at}};
}

inline json to_json_submission(const Submission& s) {
    json j;
    j["id"] = s.id;
    j["user_id"] = s.user_id;
    j["problem_id"] = s.problem_id;
    j["code"] = s.code;
    j["status"] = s.status;
    if (s.failed_case.has_value()) j["failed_case"] = s.failed_case.value();
    else j["failed_case"] = nullptr;
    if (s.time_used_ms.has_value()) j["time_used_ms"] = s.time_used_ms.value();
    else j["time_used_ms"] = nullptr;
    if (s.memory_used_kb.has_value()) j["memory_used_kb"] = s.memory_used_kb.value();
    else j["memory_used_kb"] = nullptr;
    j["passed_cases"] = s.passed_cases;
    j["total_cases"] = s.total_cases;
    if (s.compile_error.has_value()) j["compile_error"] = s.compile_error.value();
    else j["compile_error"] = nullptr;
    if (s.input_data.has_value()) j["input_data"] = s.input_data.value();
    else j["input_data"] = nullptr;
    if (s.expected_output.has_value()) j["expected_output"] = s.expected_output.value();
    else j["expected_output"] = nullptr;
    if (s.actual_output.has_value()) j["actual_output"] = s.actual_output.value();
    else j["actual_output"] = nullptr;
    j["submitted_at"] = s.submitted_at;
    if (s.judged_at.has_value()) j["judged_at"] = s.judged_at.value();
    else j["judged_at"] = nullptr;
    return j;
}

inline json to_json_leaderboard(const LeaderboardEntry& e) {
    return json{{"user_id", e.user_id}, {"username", e.username},
                {"solved_count", e.solved_count}, {"pass_rate", e.pass_rate}};
}

inline json to_json_stats(const UserStats& s) {
    return json{{"solved_count", s.solved_count},
                {"total_submissions", s.total_submissions},
                {"pass_rate", s.pass_rate},
                {"easy_count", s.easy_count},
                {"medium_count", s.medium_count},
                {"hard_count", s.hard_count}};
}
