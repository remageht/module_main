
##include "Database.h"
#include <iostream>

Database::Database(const std::string& pg_connection_string, const std::string& redis_host, int redis_port)
    : pg_conn(pg_connection_string) {
    // Подключение к PostgreSQL
    try {
        if (pg_conn.is_open()) {
            std::cout << "Connected to PostgreSQL" << std::endl;
        } else {
            throw std::runtime_error("Can't open PostgreSQL database");
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    // Подключение к Redis
    redis_conn = redisConnect(redis_host.c_str(), redis_port);
    if (redis_conn != nullptr && redis_conn->err) {
        throw std::runtime_error("Redis connection error: " + std::string(redis_conn->errstr));
    } else {
        std::cout << "Connected to Redis" << std::endl;
    }
}

Database::~Database() {
    if (redis_conn != nullptr) {
        redisFree(redis_conn);
    }
}

void Database::createTest(const std::string& name) {
    pqxx::work txn(pg_conn);
    txn.exec0("INSERT INTO tests (name) VALUES (" + txn.quote(name) + ")");
    txn.commit();
}

std::string Database::getRedisValue(const std::string& key) {
    redisReply* reply = (redisReply*)redisCommand(redis_conn, "GET %s", key.c_str());
    std::string value;
    if (reply->type == REDIS_REPLY_STRING) {
        value = reply->str;
    }
    freeReplyObject(reply);
    return value;
}

void Database::setRedisValue(const std::string& key, const std::string& value) {
    redisCommand(redis_conn, "SET %s %s", key.c_str(), value.c_str());
}
