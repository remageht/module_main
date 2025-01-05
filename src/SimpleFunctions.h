#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include <iomanip>
#include <winsock2.h>
#include <string>
#include <iostream>


inline void beautyPrint(SOCKET soc, std::string text)
{
    std::cout << '[' << std::setfill(' ') << std::setw(7) << soc << "] " << text << std::endl;
}