#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <optional>

using json = nlohmann::json;

struct JudgeTask {
    int submission_id;
    std::string code;
    std::string language;
    int problem_id;
    int time_limit;
    int memory_limit;
};

struct TestCase {
    int case_number;
    std::string input_data;
    std::string expected_output;
};

struct JudgeResult {
    int submission_id;
    std::string status;
    int passed_cases;
    int total_cases;
    int time_used_ms;
    int memory_used_kb;
    int failed_case;
    std::string compile_error;
    std::string input_data;
    std::string expected_output;
    std::string actual_output;
};
