
#include "include/Database.h"
#include "include/API.h"

int main(){
      
    Database db("postgresql://user:password@localhost/dbname");// Инициализация базы данных

    // Инициализация API
    API api(8080);
    api.start();


    return 0;
}