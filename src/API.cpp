#include "API.h"
#include <nlohmann/json.hpp>

API::API(int port, Database& db) : db(db) {
    setupRoutes();  // Настройка маршрутов перед запуском сервера
}

void API::setupRoutes() {
    svr.Post("/tests", [&](const httplib::Request& req, httplib::Response& res) {
        auto json = nlohmann::json::parse(req.body);
        std::string test_name = json["name"];
        
        db.createTest(test_name); // Метод createTest в классе Database
        
        res.set_content("Test created", "text/plain");
    });
}

void API::start() {
    svr.listen("0.0.0.0", 8080);  // Запуск сервера на указанном порту
}
