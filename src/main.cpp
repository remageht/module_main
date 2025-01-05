#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include "handleClient.h"    // Работа с клиентом

#include <winsock2.h>
#include <vector>
#include <string>
#include <thread>
#include <iostream>

const char* ADRES = "127.0.0.1";    // Локальный адрес, можно будет подключиться с другого устройства
#define PORT  1111

int main() {
    // ЗАГРУЗКА СЕТЕВОЙ БИБЛИОТЕКИ
    std::cout << "Load lib...       ";

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "Error.\n";
        return 1;
    } else {
        std::cout << "Done.\n";
    }

    // СОЗДАНИЕ СОКЕТА СЕРВЕРА
    std::cout << "Create socket...  ";

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Error.\n";
        WSACleanup();
        return 1;
    } else {
        std::cout << "Done.\n";
    }

    
    std::cout << "\nServer started\n\n";
    
    // Закрытие всех потоков перед завершением работы
    for (auto& thread : clientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
