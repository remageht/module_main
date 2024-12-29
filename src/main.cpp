#include <iostream>
#include "Database.h"
#include "API.h"
#include "third_party/cpp-httplib/httplib.h"
int main(){
      
    Database db("postgresql://user:password@localhost/dbname");// Инициализация базы данных

    // Инициализация API
    API api(8080);
    api.start();


    return 0;
}