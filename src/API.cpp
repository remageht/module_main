
#include "include/API.h"

API::API(int port) : port(port) {}

void API::start() {
    svr.Get("/hello", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("Hello, World!", "text/plain");
    });

    svr.listen("0.0.0.0", port);
}