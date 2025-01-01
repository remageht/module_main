#define CATCH_CONFIG_MAIN  // Эта строка указывает Catch2 создать функцию main
#include <catch2/catch.hpp>
#include "Database.h"

TEST_CASE("Database connection") {
    Database db("postgresql://user:password@localhost/dbname", "127.0.0.1", 6379);
    REQUIRE_NOTHROW(db.createTest("Sample Test"));
}
