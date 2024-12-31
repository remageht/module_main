#define CATCH_CONFIG_MAIN  // Эта строка указывает Catch2 создать функцию main
#include <catch2/catch.hpp>
#include "include/Database.h"

TEST_CASE("Database connection test", "[database]") {
    // Проверка подключения к базе данных
    Database db("postgresql://user:password@localhost/dbname");
    REQUIRE(db.isConnected() == true);
}

TEST_CASE("Database query test", "[database]") {
    Database db("postgresql://user:password@localhost/dbname");
    REQUIRE(db.isConnected() == true);

    // Проверка выполнения SQL-запроса
    auto result = db.execute("SELECT 1");
    REQUIRE(result.size() == 1);
    REQUIRE(result[0][0].as<int>() == 1);
}