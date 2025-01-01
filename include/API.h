
#ifndef API_H
#define API_H

#include <httplib.h>  

class API {
public:
    API(int port);
    void start();
    // Другие методы для обработки запросов
private:
    httplib::Server svr;
};

#endif 
