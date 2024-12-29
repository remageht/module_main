#ifndef DATABASE_H
#define DATABASE_H

#include <pqxx/pqxx>
#include <string>

class Database {
public:
    Database(const std::string& connectionString);
    void execute(const std::string& query);

private:
    pqxx::connection conn;
};

#endif 