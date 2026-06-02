#include "compiler.hpp"
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <fstream>
#include <iostream>

static std::string work_dir(int submission_id) {
    return "/tmp/judge/" + std::to_string(submission_id);
}

CompileResult compile(int submission_id, const std::string& code) {
    std::string dir = work_dir(submission_id);
    mkdir(dir.c_str(), 0755);

    std::string code_path = dir + "/code.cpp";
    std::ofstream(code_path) << code;

    std::string err_path = dir + "/compile_error.txt";
    std::string cmd =
        "docker run --rm --cpus=1 --memory=256m --network=none "
        "--pids-limit=50 --cap-drop=ALL "
        "-v " + dir + ":/app "
        "docker.m.daocloud.io/library/gcc:13 "
        "g++ -O2 /app/code.cpp -o /app/solution 2>" + err_path;

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
