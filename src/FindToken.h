#pragma once

#include <string>
#include <iostream>


inline std::string findToken(const std::string& message)
{
    static const size_t kMaxTokenLen = 8192;

    size_t bearerPos = message.find("Bearer ");
    if (bearerPos == std::string::npos) {
        return "";
    }

    size_t start = bearerPos + 7;
    if (start >= message.size()) {
        return "";
    }

    // Токен заканчивается на первом разделителе: пробел, \n, \r, \t или '"'
    size_t cut_pos = message.size();
    for (char delim : {' ', '\n', '\r', '\t', '"', '\''}) {
        size_t p = message.find(delim, start);
        if (p != std::string::npos && p < cut_pos) {
            cut_pos = p;
        }
    }

    std::string token = message.substr(start, cut_pos - start);

    // Отсекаем возможный префикс "Bearer " повторно и ограничиваем длину
    if (token.size() > kMaxTokenLen) {
        token.resize(kMaxTokenLen);
    }

    return token;
}