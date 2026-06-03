#pragma once

#include <string>

struct CompileResult {
    bool success;
    std::string error;
};

CompileResult compile(int submission_id, const std::string& code, const std::string& language = "cpp");
