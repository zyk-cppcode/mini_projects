#include "queue.hpp"
#include <hiredis/hiredis.h>
#include <stdexcept>

static redisContext* redis_ctx = nullptr;

bool connect_redis(const std::string& host, int port) {
    redis_ctx = redisConnect(host.c_str(), port);
    if (!redis_ctx || redis_ctx->err) {
        std::string err = redis_ctx ? redis_ctx->errstr : "unknown";
        redisFree(redis_ctx);
        redis_ctx = nullptr;
        throw std::runtime_error("Redis connect failed: " + err);
    }
    return true;
}

std::optional<JudgeTask> fetch_task(int timeout_seconds) {
    if (!redis_ctx) return std::nullopt;

    redisReply* reply = (redisReply*)redisCommand(redis_ctx,
        "BRPOP submit_queue %d", timeout_seconds);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return std::nullopt;
    }

    if (reply->type == REDIS_REPLY_NIL || reply->elements < 2) {
        freeReplyObject(reply);
        return std::nullopt;
    }

    std::string payload(reply->element[1]->str, reply->element[1]->len);
    freeReplyObject(reply);

    try {
        json j = json::parse(payload);
        JudgeTask task;
        task.submission_id = j["submission_id"];
        task.code = j["code"];
        task.problem_id = j["problem_id"];
        task.time_limit = j.value("time_limit", 2000);
        task.memory_limit = j.value("memory_limit", 262144);
        return task;
    } catch (...) {
        return std::nullopt;
    }
}
