#pragma once

#include <httplib.h>

void handle_get_problems(const httplib::Request& req, httplib::Response& res);
void handle_get_problem(const httplib::Request& req, httplib::Response& res);
void handle_get_testdata(const httplib::Request& req, httplib::Response& res);
