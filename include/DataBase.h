#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <pqxx/pqxx>
#include <hiredis/hiredis.h>
#include <nlohmann/json.hpp>

class Database {
public:
    Database(const std::string& pg_connection_string, const std::string& redis_host, int redis_port);
    ~Database();

    void createTest(const std::string& name);
    std::string getRedisValue(const std::string& key);
    void setRedisValue(const std::string& key, const std::string& value);
    // Другие методы для работы с базой данных

private:
    pqxx::connection pg_conn;
    redisContext* redis_conn;
};

#endif 
