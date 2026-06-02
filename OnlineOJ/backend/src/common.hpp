#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "model.hpp"
#include "middleware.hpp"

static inline std::string extract_session(const httplib::Request& req) {
    auto it = req.headers.find("Cookie");
    if (it == req.headers.end()) return "";
    const std::string& cookie = it->second;
    size_t pos = cookie.find("session=");
    if (pos == std::string::npos) return "";
    pos += 8;
    size_t end = cookie.find(';', pos);
    if (end == std::string::npos) end = cookie.size();
    return cookie.substr(pos, end - pos);
}

static inline void json_response(httplib::Response& res, const json& data, int status = 200) {
    res.status = status;
    res.set_content(data.dump(), "application/json");
}

static inline void error_response(httplib::Response& res, int status,
                                   const std::string& code, const std::string& msg) {
    json j;
    j["error"] = msg;
    j["code"] = code;
    json_response(res, j, status);
}
