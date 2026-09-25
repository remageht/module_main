#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include "HandleClient.h"	//		работа с клиентом
#include "getAddress.h"		//		получение айпи устройства в локальной сети

#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <cstdlib>
#include <string>
#include <thread>
#include <iostream>

const char* ADRES = "127.0.0.1";	//	локалхост в коде меняется на текущий адрес устройства в локальной сети (можно будет подключиться с другого устройства)
#define PORT  1111
#define MAX_CLIENT_THREADS 128



int main()
{
	//		ПРОВЕРКА КОНФИГУРАЦИИ (secrets только через env, fail fast с понятной ошибкой)
	if (std::getenv("JWT_SECRET") == nullptr) {
		std::cerr << "WARNING: JWT_SECRET is not set. Token verification will reject all requests (fail closed)." << std::endl;
		std::cerr << "Set JWT_SECRET env var, see .env.example." << std::endl;
	}
	//		ЗАГРУЗКА СЕТЕВОЙ БИБЛИОТЕКИ

	std::cout << "Load _lib...       ";

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cout << "Error.\n";
		return 1;
	}
	else {
		std::cout << "Done.\n";
	}


	//		СОЗДАНИЕ СОКЕТА СЕРВЕРА

	std::cout << "Create socket...  ";

	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET) {
		std::cout << "Error.\n";
		WSACleanup();
		return 1;
	}
	else {
		std::cout << "Done.\n";
	}


	//		ОПРЕДЕЛЕНИЕ АДРЕСА
	//		если не вызывать сервер запустится на локалхосте

	//		ADDRESS = getLocalIPAddress();


	//		ПРИСВАИВАНИЕ АДРЕСА И ПОРТА

	std::cout << "Address and port... ";

	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	// inet_pton вместо deprecated inet_addr, с проверкой результата.
	if (inet_pton(AF_INET, ADRES, &serverAddr.sin_addr) != 1) {
		std::cout << "Error: bad address.\n";
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}
	serverAddr.sin_port = htons(PORT);
	if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cout << "Error: bind failed (" << WSAGetLastError() << ").\n";
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	std::cout << "Done.\n";


	//		ПРОСЛУШИВАНИЕ ПОРТА(сервер считается запущенным)
	

	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cout << "Error: listen failed (" << WSAGetLastError() << ").\n";
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	std::cout << "\nServer started" << std::endl << std::endl;
	std::cout << "Address - " << ADRES << std::endl;
	std::cout << "Port    - " << PORT << std::endl;
	std::cout << "Link    - " << "http://" << ADRES << ':' << PORT << '/' << std::endl << std::endl;


	//		ОБРАБОТКА ПОПЫТКИ ПОДКЛЮЧЕНИЯ

	std::atomic<int> activeClients{ 0 };

	while (true)
	{
		sockaddr_in clientAddr;
		int clientAddrSize = sizeof(clientAddr);
		SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
			continue;
		}

		// Защита от исчерпания ресурсов: thread-per-connection без лимита = DoS.
		if (activeClients.load() >= MAX_CLIENT_THREADS) {
			std::cerr << "Too many clients, rejecting socket " << clientSocket << std::endl;
			closesocket(clientSocket);
			continue;
		}

		// Таймаут recv против slow-loris / зависших соединений (30 c).
		DWORD recvTimeoutMs = 30000;
		setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,
			reinterpret_cast<const char*>(&recvTimeoutMs), sizeof(recvTimeoutMs));

		// Запуск нового потока для обработки клиента.
		// detach вместо вечно растущего vector<thread>: иначе terminate() на joinable thread
		// и утечка потоков. Счётчик активных клиентов ограничивает нагрузку.
		activeClients.fetch_add(1);
		std::thread([clientSocket, &activeClients]() {
			handleClient(clientSocket);
			activeClients.fetch_sub(1);
		}).detach();
	}

	closesocket(serverSocket);
	WSACleanup();

	return 0;
}