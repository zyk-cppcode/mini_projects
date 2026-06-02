#pragma once

#include <string>
#include <optional>
#include "model.hpp"

std::optional<User> auth_middleware(const std::string& session_cookie);
void require_auth(const std::optional<User>& user);
void require_admin(const std::optional<User>& user);
