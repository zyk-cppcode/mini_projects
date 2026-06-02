#pragma once

#include "interface.hpp"
#include <string>

extern std::string g_backend_url;
extern std::string g_api_token;

void set_reporter_config(const std::string& backend_url, const std::string& api_token);
bool report_result(const JudgeResult& result);
std::vector<TestCase> fetch_testdata(int problem_id);
