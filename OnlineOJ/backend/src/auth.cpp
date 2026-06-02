#include "auth.hpp"
#include "db.hpp"
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

Auth& Auth::instance() {
    static Auth auth;
    return auth;
}

void Auth::init(const std::string&) {
}

std::string Auth::hash_password(const std::string& password) {
    unsigned char salt[16];
    RAND_bytes(salt, sizeof(salt));

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, salt, sizeof(salt));
    SHA256_Update(&ctx, password.c_str(), password.size());
    SHA256_Final(hash, &ctx);

    std::ostringstream ss;
    for (int i = 0; i < 16; i++)
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)salt[i];
    ss << ":";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)hash[i];
    return ss.str();
}

bool Auth::verify_password(const std::string& password, const std::string& stored) {
    size_t pos = stored.find(':');
    if (pos == std::string::npos) return false;

    std::string salt_hex = stored.substr(0, pos);
    unsigned char salt[16];
    for (size_t i = 0; i < 16; i++) {
        unsigned int byte;
        std::stringstream ss;
        ss << std::hex << salt_hex.substr(i * 2, 2);
        ss >> byte;
        salt[i] = (unsigned char)byte;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, salt, sizeof(salt));
    SHA256_Update(&ctx, password.c_str(), password.size());
    SHA256_Final(hash, &ctx);

    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)hash[i];

    return ss.str() == stored.substr(pos + 1);
}

User Auth::register_user(const std::string& username, const std::string& password) {
    auto db = DbPool::instance().guard();

    MYSQL_RES* res = db_query(db.get(),
        "SELECT id FROM users WHERE username='" + db_escape(db.get(), username) + "'");
    if (mysql_num_rows(res) > 0) {
        mysql_free_result(res);
        throw std::runtime_error("用户名已存在");
    }
    mysql_free_result(res);

    std::string hash = hash_password(password);
    std::string sql = "INSERT INTO users (username, password_hash, role) VALUES ('"
        + db_escape(db.get(), username) + "', '"
        + db_escape(db.get(), hash) + "', 'user')";
    db_execute(db.get(), sql);

    User user;
    user.id = db_last_insert_id(db.get());
    user.username = username;
    user.role = "user";
    return user;
}

std::string Auth::login(const std::string& username, const std::string& password) {
    auto db = DbPool::instance().guard();

    std::string sql = "SELECT id, username, password_hash, role FROM users WHERE username='"
        + db_escape(db.get(), username) + "'";
    MYSQL_RES* res = db_query(db.get(), sql);
    if (mysql_num_rows(res) == 0) {
        mysql_free_result(res);
        throw std::runtime_error("用户名或密码错误");
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    int id = std::stoi(row[0]);
    std::string stored_hash = row[2] ? row[2] : "";

    if (!verify_password(password, stored_hash)) {
        mysql_free_result(res);
        throw std::runtime_error("用户名或密码错误");
    }
    mysql_free_result(res);

    std::string session_id = generate_session_id();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[session_id] = id;
    }
    return session_id;
}

void Auth::logout(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session_id);
}

std::optional<User> Auth::get_user_by_session(const std::string& session_id) {
    int user_id;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(session_id);
        if (it == sessions_.end()) return std::nullopt;
        user_id = it->second;
    }

    auto db = DbPool::instance().guard();
    std::string sql = "SELECT id, username, role, created_at FROM users WHERE id="
        + std::to_string(user_id);
    MYSQL_RES* res = db_query(db.get(), sql);
    if (mysql_num_rows(res) == 0) {
        mysql_free_result(res);
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    User user;
    user.id = std::stoi(row[0]);
    user.username = row[1] ? row[1] : "";
    user.role = row[2] ? row[2] : "user";
    user.created_at = row[3] ? row[3] : "";
    mysql_free_result(res);
    return user;
}

std::string Auth::generate_session_id() {
    unsigned char buf[32];
    RAND_bytes(buf, sizeof(buf));
    std::ostringstream ss;
    for (int i = 0; i < 32; i++)
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)buf[i];
    return ss.str();
}
