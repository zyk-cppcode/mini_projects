#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <mutex>
#include "model.hpp"

class Auth {
public:
    static Auth& instance();

    void init(const std::string& session_secret);

    User register_user(const std::string& username, const std::string& password);
    std::string login(const std::string& username, const std::string& password);
    void logout(const std::string& session_id);
    std::optional<User> get_user_by_session(const std::string& session_id);

    static std::string hash_password(const std::string& password);
    static bool verify_password(const std::string& password, const std::string& hash);

private:
    Auth() = default;

    std::string generate_session_id();
    std::unordered_map<std::string, int> sessions_;
    std::mutex mutex_;
};
