
#ifndef API_H
#define API_H

#include <httplib.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class API {
public:
    API(int port);
    void start();

private:
    httplib::Server svr;
    int port;
};

#endif 