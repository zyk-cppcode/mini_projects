#pragma once

#include <httplib.h>

void handle_leaderboard(const httplib::Request& req, httplib::Response& res);
void handle_statistics(const httplib::Request& req, httplib::Response& res);
