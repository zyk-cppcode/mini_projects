#pragma once

#include <string>

struct RunResult {
    int exit_code;
    std::string stdout_output;
    bool oom_killed;
    bool timed_out;
};

RunResult run_test(int submission_id, const std::string& input_data,
                   int time_limit_ms, int memory_limit_kb);
void cleanup_work_dir(int submission_id);
