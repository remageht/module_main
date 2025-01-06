#pragma once

#include <string>
#include <iostream>


inline std::string findToken(std::string message)
{
    std::string token = "";

    size_t bearerPos = message.find("Bearer ");
    if (bearerPos != std::string::npos) {
        token = message.substr(bearerPos + 7);
    }

    size_t cut_pos = token.size();
    if (token.find(' ') != std::string::npos) {
        if (cut_pos > token.find(' '))
            cut_pos = token.find(' ');
    }
    if (token.find(' ') != std::string::npos) {
        if (cut_pos > token.find('\n'))
            cut_pos = token.find('\n');
    }
    if (token.find(' ') != std::string::npos) {
        if (cut_pos > token.find('\r'))
            cut_pos = token.find('\r');
    }

    token = token.substr(0, cut_pos);

    return token;
}