#include "compiler.hpp"
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <fstream>
#include <iostream>

static std::string work_dir(int submission_id) {
    return "/tmp/judge/" + std::to_string(submission_id);
}

static bool use_docker() {
    static int checked = 0;
    static int available = 0;
    if (!checked) {
        checked = 1;
        available = (system("docker image inspect docker.m.daocloud.io/library/gcc:13 > /dev/null 2>&1") == 0);
    }
    return available;
}

CompileResult compile(int submission_id, const std::string& code, const std::string& language) {
    std::string dir = work_dir(submission_id);
    mkdir(dir.c_str(), 0755);

    if (language == "python") {
        std::string code_path = dir + "/solution.py";
        std::ofstream(code_path) << code;
        std::string err_path = dir + "/compile_error.txt";
        std::string cmd = "python3 -m py_compile " + code_path + " 2>" + err_path;
        int ret = system(cmd.c_str());
        CompileResult result;
        if (ret != 0) {
            result.success = false;
            std::ifstream err_file(err_path);
            result.error = std::string(std::istreambuf_iterator<char>(err_file),
                                        std::istreambuf_iterator<char>());
            if (result.error.empty()) result.error = "Syntax error";
        } else {
            result.success = true;
        }
        return result;
    }

    std::string code_path = dir + "/code.cpp";
    std::ofstream(code_path) << code;

    std::string err_path = dir + "/compile_error.txt";
    std::string cmd;
    if (use_docker()) {
        cmd =
            "docker run --rm --cpus=1 --memory=256m --network=none "
            "--pids-limit=50 --cap-drop=ALL "
            "-v " + dir + ":/app "
            "docker.m.daocloud.io/library/gcc:13 "
            "g++ -O2 /app/code.cpp -o /app/solution 2>" + err_path;
    } else {
        cmd = "g++ -std=c++17 -O2 " + code_path + " -o " + dir + "/solution 2>" + err_path;
    }

    int ret = system(cmd.c_str());

    CompileResult result;
    if (ret != 0) {
        result.success = false;
        std::ifstream err_file(err_path);
        result.error = std::string(std::istreambuf_iterator<char>(err_file),
                                    std::istreambuf_iterator<char>());
        if (result.error.empty()) result.error = "Compilation failed with exit code " + std::to_string(ret);
    } else {
        struct stat st;
        result.success = (stat((dir + "/solution").c_str(), &st) == 0);
        if (!result.success) result.error = "Binary not generated";
    }
    return result;
}
