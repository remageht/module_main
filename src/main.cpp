#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include "HandleClient.h"	//		работа с клиентом
#include "GetAddress.h"		//		получение айпи устройства в локальной сети

#include <winsock2.h>
#include <vector>
#include <string>
#include <thread>
#include <iostream>

const char* ADRES = "127.0.0.1";	//	локалхост в коде меняется на текущий адрес устройства в локальной сети (можно будет подключиться с другого устройства)
#define PORT  1111



int main()
{
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
	serverAddr.sin_addr.s_addr = inet_addr(ADRES);
	serverAddr.sin_port = htons(PORT);
	bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

	std::cout << "Done.\n";


	//		ПРОСЛУШИВАНИЕ ПОРТА(сервер считается запущенным)
	

	listen(serverSocket, SOMAXCONN);

	std::cout << "\nServer started" << std::endl << std::endl;
	std::cout << "Address - " << ADRES << std::endl;
	std::cout << "Port    - " << PORT << std::endl;
	std::cout << "Link    - " << "http://" << ADRES << ':' << PORT << '/' << std::endl << std::endl;


	//		ОБРАБОТКА ПОПЫТКИ ПОДКЛЮЧЕНИЯ

	std::vector<std::thread> clientThreads;

	while (true)
	{
		sockaddr_in clientAddr;
		int clientAddrSize = sizeof(clientAddr);
		SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
			continue;
		}

		// Запуск нового потока для обработки клиента
		clientThreads.emplace_back(std::thread(handleClient, clientSocket));
	}

	std::cout << "test";

	return 0;
}