#pragma once

#include <httplib.h>

void handle_submit(const httplib::Request& req, httplib::Response& res);
void handle_get_submission(const httplib::Request& req, httplib::Response& res);
void handle_get_submissions(const httplib::Request& req, httplib::Response& res);
void handle_report_result(const httplib::Request& req, httplib::Response& res);
