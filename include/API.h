
#ifndef API_H
#define API_H

#include <httplib.h>
#include "Database.h"

class API {
public:
    API(int port, Database& db);
    void start();

private:
    httplib::Server svr;
    Database& db;

    void setupRoutes();
};

#endif 
