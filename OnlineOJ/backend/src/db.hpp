#pragma once

#include <mysql/mysql.h>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include <stdexcept>

class DbPool {
public:
    static DbPool& instance();

    void init(const std::string& host, int port,
              const std::string& user, const std::string& pass,
              const std::string& db, int pool_size = 10);

    MYSQL* acquire();
    void release(MYSQL* conn);

    class Guard {
    public:
        Guard(MYSQL* c, DbPool& p) : conn(c), pool(p) {}
        ~Guard() { pool.release(conn); }
        MYSQL* operator->() { return conn; }
        MYSQL* get() { return conn; }
    private:
        MYSQL* conn;
        DbPool& pool;
    };
    Guard guard();

private:
    DbPool() = default;
    ~DbPool();
    DbPool(const DbPool&) = delete;
    DbPool& operator=(const DbPool&) = delete;

    std::mutex mutex_;
    std::queue<MYSQL*> pool_;
    std::string host_, user_, pass_, db_;
    int port_ = 3306;
    bool initialized_ = false;
};

MYSQL_RES* db_query(MYSQL* conn, const std::string& sql);
int db_execute(MYSQL* conn, const std::string& sql);
std::string db_escape(MYSQL* conn, const std::string& str);
long long db_last_insert_id(MYSQL* conn);
