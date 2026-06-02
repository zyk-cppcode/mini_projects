#include "middleware.hpp"
#include "auth.hpp"
#include <stdexcept>

std::optional<User> auth_middleware(const std::string& session_cookie) {
    if (session_cookie.empty()) return std::nullopt;
    return Auth::instance().get_user_by_session(session_cookie);
}

void require_auth(const std::optional<User>& user) {
    if (!user.has_value()) {
        throw std::runtime_error("UNAUTHORIZED");
    }
}

void require_admin(const std::optional<User>& user) {
    require_auth(user);
    if (user->role != "admin") {
        throw std::runtime_error("FORBIDDEN");
    }
}
