#pragma once

#include "interface.hpp"
#include <string>

bool connect_redis(const std::string& host, int port);
std::optional<JudgeTask> fetch_task(int timeout_seconds = 5);
