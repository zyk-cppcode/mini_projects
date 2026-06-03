#include "queue.hpp"
#include "compiler.hpp"
#include "runner.hpp"
#include "comparator.hpp"
#include "reporter.hpp"
#include <iostream>
#include <cstdlib>
#include <signal.h>

static JudgeResult judge_task(const JudgeTask& task) {
    JudgeResult result;
    result.submission_id = task.submission_id;
    result.status = "system_error";
    result.passed_cases = 0;
    result.total_cases = 0;
    result.time_used_ms = 0;
    result.memory_used_kb = 0;
    result.failed_case = 0;

    CompileResult cr = compile(task.submission_id, task.code, task.language);
    if (!cr.success) {
        result.status = "compilation_error";
        result.compile_error = cr.error;
        cleanup_work_dir(task.submission_id);
        return result;
    }

    auto testcases = fetch_testdata(task.problem_id);
    if (testcases.empty()) {
        result.status = "system_error";
        result.compile_error = "No test data found";
        cleanup_work_dir(task.submission_id);
        return result;
    }

    result.total_cases = testcases.size();
    int total_time = 0;

    for (const auto& tc : testcases) {
        RunResult rr = run_test(task.submission_id, tc.input_data,
                                task.time_limit, task.memory_limit, task.language);

        if (rr.timed_out) {
            result.status = "time_limit_exceeded";
            result.failed_case = tc.case_number;
            result.passed_cases = tc.case_number - 1;
            result.input_data = tc.input_data;
            result.expected_output = tc.expected_output;
            cleanup_work_dir(task.submission_id);
            return result;
        }

        if (rr.oom_killed) {
            result.status = "memory_limit_exceeded";
            result.failed_case = tc.case_number;
            result.passed_cases = tc.case_number - 1;
            result.input_data = tc.input_data;
            result.expected_output = tc.expected_output;
            cleanup_work_dir(task.submission_id);
            return result;
        }

        if (rr.exit_code != 0) {
            result.status = "wrong_answer";
            result.failed_case = tc.case_number;
            result.passed_cases = tc.case_number - 1;
            result.input_data = tc.input_data;
            result.expected_output = tc.expected_output;
            result.actual_output = rr.stdout_output;
            cleanup_work_dir(task.submission_id);
            return result;
        }

        if (!compare_output(rr.stdout_output, tc.expected_output)) {
            result.status = "wrong_answer";
            result.failed_case = tc.case_number;
            result.passed_cases = tc.case_number - 1;
            result.input_data = tc.input_data;
            result.expected_output = tc.expected_output;
            result.actual_output = rr.stdout_output;
            cleanup_work_dir(task.submission_id);
            return result;
        }

        total_time += task.time_limit;
        result.passed_cases = tc.case_number;
    }

    result.status = "accepted";
    result.time_used_ms = total_time;
    result.memory_used_kb = 0;
    cleanup_work_dir(task.submission_id);
    return result;
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    const char* env_redis_host = std::getenv("REDIS_HOST");
    const char* env_redis_port = std::getenv("REDIS_PORT");
    const char* env_backend_url = std::getenv("JUDGE_BACKEND_URL");
    const char* env_api_token = std::getenv("JUDGE_API_TOKEN");

    std::string redis_host = env_redis_host ? env_redis_host : "localhost";
    int redis_port = env_redis_port ? std::stoi(env_redis_port) : 6379;
    std::string backend_url = env_backend_url ? env_backend_url : "http://localhost:8080";
    g_api_token = env_api_token ? env_api_token : "";
    g_backend_url = backend_url;

    set_reporter_config(backend_url, g_api_token);

    std::cout << "Judge connecting to Redis " << redis_host << ":" << redis_port << std::endl;
    try {
        connect_redis(redis_host, redis_port);
        std::cout << "Redis connected." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Redis connect failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Judge started, waiting for tasks..." << std::endl;
    while (true) {
        auto task = fetch_task(5);
        if (!task.has_value()) continue;

        std::cout << "Processing submission " << task->submission_id
                  << " (problem " << task->problem_id << ")" << std::endl;

        JudgeResult result = judge_task(*task);
        bool ok = report_result(result);
        std::cout << "  Result: " << result.status
                  << " (" << result.passed_cases << "/" << result.total_cases << ")"
                  << " reported: " << (ok ? "ok" : "FAIL") << std::endl;
    }

    return 0;
}
