#include "comparator.hpp"
#include <algorithm>
#include <sstream>

static std::string strip_trailing_spaces(const std::string& s) {
    size_t end = s.find_last_not_of(" \t");
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

static std::string normalize(const std::string& s) {
    std::string result = s;
    size_t pos = 0;
    while ((pos = result.find("\r\n", pos)) != std::string::npos) {
        result.replace(pos, 2, "\n");
        pos += 1;
    }
    pos = 0;
    while ((pos = result.find('\r', pos)) != std::string::npos) {
        result.replace(pos, 1, "\n");
        pos += 1;
    }

    std::istringstream iss(result);
    std::ostringstream oss;
    std::string line;
    while (std::getline(iss, line)) {
        oss << strip_trailing_spaces(line) << "\n";
    }
    return oss.str();
}

bool compare_output(const std::string& actual, const std::string& expected) {
    return normalize(actual) == normalize(expected);
}
