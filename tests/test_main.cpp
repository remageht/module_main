#include <cassert>
#include "include/Database.h"

void testDatabase() {
    Database db("postgresql://user:password@localhost/dbname");
    db.execute("CREATE TABLE IF NOT EXISTS test (id SERIAL PRIMARY KEY, name TEXT);");
    db.execute("INSERT INTO test (name) VALUES ('Test Name');");
    // Добавить проверку результатов
}

int main() {
    testDatabase();
    return 0;
}