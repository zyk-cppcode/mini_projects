#include "db.hpp"
#include <thread>
#include <chrono>

DbPool& DbPool::instance() {
    static DbPool pool;
    return pool;
}

DbPool::~DbPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!pool_.empty()) {
        MYSQL* conn = pool_.front();
        pool_.pop();
        mysql_close(conn);
    }
}

void DbPool::init(const std::string& host, int port,
                  const std::string& user, const std::string& pass,
                  const std::string& db, int pool_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    host_ = host;
    port_ = port;
    user_ = user;
    pass_ = pass;
    db_ = db;

    for (int i = 0; i < pool_size; i++) {
        MYSQL* conn = mysql_init(nullptr);
        if (!conn) throw std::runtime_error("mysql_init failed");

        mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");
        mysql_options(conn, MYSQL_OPT_RECONNECT, "1");

        if (!mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(),
                                db.c_str(), port, nullptr, 0)) {
            std::string err = mysql_error(conn);
            mysql_close(conn);
            throw std::runtime_error("mysql_real_connect failed: " + err);
        }
        pool_.push(conn);
    }
    initialized_ = true;
}

MYSQL* DbPool::acquire() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (pool_.empty()) {
        mutex_.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        mutex_.lock();
    }
    MYSQL* conn = pool_.front();
    pool_.pop();
    mysql_ping(conn);
    return conn;
}

void DbPool::release(MYSQL* conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.push(conn);
}

DbPool::Guard DbPool::guard() {
    return Guard(acquire(), *this);
}

MYSQL_RES* db_query(MYSQL* conn, const std::string& sql) {
    if (mysql_query(conn, sql.c_str()) != 0) {
        throw std::runtime_error("MySQL query error: " + std::string(mysql_error(conn)));
    }
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result && mysql_errno(conn) != 0) {
        throw std::runtime_error("MySQL store result error: " + std::string(mysql_error(conn)));
    }
    return result;
}

int db_execute(MYSQL* conn, const std::string& sql) {
    if (mysql_query(conn, sql.c_str()) != 0) {
        throw std::runtime_error("MySQL execute error: " + std::string(mysql_error(conn)));
    }
    return mysql_affected_rows(conn);
}

std::string db_escape(MYSQL* conn, const std::string& str) {
    std::vector<char> buf(str.size() * 2 + 1);
    mysql_real_escape_string(conn, buf.data(), str.c_str(), str.size());
    return std::string(buf.data());
}

long long db_last_insert_id(MYSQL* conn) {
    return mysql_insert_id(conn);
}
