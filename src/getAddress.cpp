
#include "getAddress.h"
#include <iostream>
#include <vector>
#include <thread>

std::string getLocalIPAddress() {
	std::string fallback = "127.0.0.1";

	char hostname[256] = {0};
	if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
		return fallback;
	}

	std::cout << std::endl;
	std::cout << "Hostname: " << hostname << std::endl;

	// getaddrinfo: потокобезопасно, поддерживает IPv4/IPv6.
	struct addrinfo hints = {};
	hints.ai_family = AF_INET; // только IPv4 для совместимости с остальным кодом
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo* result = nullptr;
	if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) {
		return fallback;
	}

	std::string address = fallback;
	std::cout << "Local IP Addresses:" << std::endl;
	for (struct addrinfo* p = result; p != nullptr; p = p->ai_next) {
		const auto* ipv4 = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
		char ipStr[INET_ADDRSTRLEN] = {0};
		if (inet_ntop(AF_INET, &ipv4->sin_addr, ipStr, sizeof(ipStr)) != nullptr) {
			std::cout << "- " << ipStr << std::endl;
			address = ipStr;
		}
	}
	std::cout << std::endl;

	freeaddrinfo(result);

	return address;
}