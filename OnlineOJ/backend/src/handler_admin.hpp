#pragma once

#include <httplib.h>

void handle_admin_create_problem(const httplib::Request& req, httplib::Response& res);
void handle_admin_update_problem(const httplib::Request& req, httplib::Response& res);
void handle_admin_delete_problem(const httplib::Request& req, httplib::Response& res);
void handle_admin_upload_testdata(const httplib::Request& req, httplib::Response& res);
void handle_admin_tags(const httplib::Request& req, httplib::Response& res);
