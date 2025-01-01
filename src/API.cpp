
#include "API.h"

API::API(int port) {
    // Инициализация сервера на указанном порту
    svr.listen("0.0.0.0", port);
}

void API::start() {
    // Запуск сервера
    svr.listen();
}