#include "server.hpp"
#include "db.hpp"
#include "auth.hpp"
#include "rate_limiter.hpp"
#include <httplib.h>
#include <cstdlib>
#include <iostream>
#include <signal.h>

extern void init_redis(const std::string& host, int port);

int main() {
    signal(SIGPIPE, SIG_IGN);

    const char* env_host = std::getenv("MYSQL_HOST");
    const char* env_port = std::getenv("MYSQL_PORT");
    const char* env_user = std::getenv("MYSQL_USER");
    const char* env_pass = std::getenv("MYSQL_PASSWORD");
    const char* env_db = std::getenv("MYSQL_DATABASE");
    const char* env_redis_host = std::getenv("REDIS_HOST");
    const char* env_redis_port = std::getenv("REDIS_PORT");
    const char* env_backend_port = std::getenv("BACKEND_PORT");
    const char* env_rate_limit = std::getenv("RATE_LIMIT_PER_MIN");

    std::string mysql_host = env_host ? env_host : "localhost";
    int mysql_port = env_port ? std::stoi(env_port) : 3306;
    std::string mysql_user = env_user ? env_user : "root";
    std::string mysql_pass = env_pass ? env_pass : "";
    std::string mysql_db = env_db ? env_db : "onlineoj";
    std::string redis_host = env_redis_host ? env_redis_host : "localhost";
    int redis_port = env_redis_port ? std::stoi(env_redis_port) : 6379;
    int backend_port = env_backend_port ? std::stoi(env_backend_port) : 8080;
    int rate_limit = env_rate_limit ? std::stoi(env_rate_limit) : 10;

    std::cout << "Connecting to MySQL " << mysql_host << ":" << mysql_port << "..." << std::endl;
    try {
        DbPool::instance().init(mysql_host, mysql_port, mysql_user, mysql_pass, mysql_db, 10);
        std::cout << "MySQL connected." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "MySQL init failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Connecting to Redis " << redis_host << ":" << redis_port << "..." << std::endl;
    try {
        init_redis(redis_host, redis_port);
        std::cout << "Redis connected." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Warning: Redis init failed: " << e.what() << std::endl;
        std::cerr << "Submit/judge functionality will be unavailable." << std::endl;
    }

    Auth::instance().init("");
    RateLimiter::instance().init(rate_limit);

    httplib::Server server;
    setup_server(server);

    std::cout << "Backend listening on 0.0.0.0:" << backend_port << std::endl;
    server.listen("0.0.0.0", backend_port);

    return 0;
}
