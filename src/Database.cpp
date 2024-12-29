#include <iostream>
#include "Database.h"

Database::Database(const std::string& connectionString) : conn(connectionString) {}

void Database::execute(const std::string& query) {
    pqxx::work txn(conn);
    txn.exec(query);
    txn.commit();
}