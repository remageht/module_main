#include "HandleClient.h"
#include "API.h"

int main() {
    Database db("postgresql://user:password@localhost/dbname", "127.0.0.1", 6379); // Инициализация базы данных и Redis

    // Инициализация API
    API api(8080, db);
    api.start();

    return 0;
}
