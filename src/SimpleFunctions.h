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

// Маскирование секретов для логов: не пишем токен целиком (PII/secrets).
// Показываем первые 6 символов + длину, чтобы можно было отлаживать без утечки.
inline std::string maskToken(const std::string& token)
{
    if (token.empty()) {
        return "<empty>";
    }
    std::string head = token.substr(0, token.size() < 6 ? token.size() : 6);
    return head + "...<len=" + std::to_string(token.size()) + ">";
}