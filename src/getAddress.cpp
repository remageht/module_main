
#include "getAddress.h"
#include <iostream>
#include <vector>
#include <thread>

const char* getLocalIPAddress() {
	const char* address = "127.0.0.1";

	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
		return address;
	}

	std::cout << std::endl;
	std::cout << "Hostname: " << hostname << std::endl;

	// Получение IP-адреса по имени хоста
	struct hostent* hostInfo = gethostbyname(hostname);
	if (hostInfo == nullptr) {
		return address;
	}

	std::cout << "Local IP Addresses:" << std::endl;
	for (int i = 0; hostInfo->h_addr_list[i] != nullptr; i++) {
		struct in_addr addr;
		memcpy(&addr, hostInfo->h_addr_list[i], sizeof(struct in_addr));
		std::cout << "- " << inet_ntoa(addr) << std::endl;
		address = inet_ntoa(addr);
	}
	std::cout << std::endl;

	return address;
}