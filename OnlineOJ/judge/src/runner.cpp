#include "runner.hpp"
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>

static std::string work_dir(int submission_id) {
    return "/tmp/judge/" + std::to_string(submission_id);
}

RunResult run_test(int submission_id, const std::string& input_data,
                   int time_limit_ms, int memory_limit_kb) {
    std::string dir = work_dir(submission_id);
    int timeout_sec = (time_limit_ms + 999) / 1000;
    if (timeout_sec < 1) timeout_sec = 1;
    int mem_mb = (memory_limit_kb + 1023) / 1024;
    if (mem_mb < 16) mem_mb = 16;

    std::string in_path = dir + "/stdin.txt";
    std::string out_path = dir + "/stdout.txt";
    std::string err_path = dir + "/stderr.txt";

    std::ofstream(in_path) << input_data;

    std::ostringstream cmd;
    cmd << "docker run --rm --cpus=1 --memory=" << mem_mb << "m "
        << "--network=none --pids-limit=50 --cap-drop=ALL "
        << "-v " << dir << ":/app "
        << "docker.m.daocloud.io/library/gcc:13 "
        << "timeout " << timeout_sec << " /app/solution "
        << "< " << in_path << " > " << out_path << " 2>" << err_path;

    int ret = system(cmd.str().c_str());
    int exit_code = WIFEXITED(ret) ? WEXITSTATUS(ret) : -1;

    RunResult result;
    result.oom_killed = (exit_code == 137);
    result.timed_out = (exit_code == 124);

    if (exit_code == 124) {
        result.exit_code = 124;
    } else if (exit_code == 137) {
        result.exit_code = 137;
    } else {
        result.exit_code = exit_code;
    }

    std::ifstream out_file(out_path);
    if (out_file) {
        std::stringstream buf;
        buf << out_file.rdbuf();
        result.stdout_output = buf.str();
    }

    return result;
}

void cleanup_work_dir(int submission_id) {
    std::string dir = work_dir(submission_id);
    std::string cmd = "rm -rf " + dir;
    system(cmd.c_str());
}
