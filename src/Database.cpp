
#include "Database.h"

Database::Database(const std::string& connection_string) 
    : conn(connection_string) {
    // Проверка соединения и инициализация
}
