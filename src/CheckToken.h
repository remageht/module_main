#pragma once

#include <jwt-cpp/jwt.h>
#include <chrono>
#include <string>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include <winsock2.h>
#include "SimpleFunctions.h"
#include "FindToken.h"


struct Permission {
    struct User {
        bool list = false;          // user:list:read
        bool info = false;          // user:info:read
        bool nameWrite = false;     // user:fullName:write
        bool data = false;          // user:data:read
        bool roleGet = false;       // user:roles:read
        bool roleSet = false;       // user:roles:write
        bool blockGet = false;      // user:block:read
        bool blockSet = false;      // user:block:write
    };

    struct Course {
        bool list = false;          // course:list:read
        bool info = false;          // course:info:read
        bool infoSet = false;       // course:info:write
        bool testList = false;      // course:testList
        bool testInfo = false;      // course:test:read
        bool testSet = false;       // course:test:write
        bool testAdd = false;       // course:test:add
        bool testDel = false;       // course:test:del
        bool userList = false;      // course:userList:read
        bool userAdd = false;       // course:user:add
        bool userDel = false;       // course:user:del
        bool courseAdd = false;     // course:add
        bool courseDel = false;     // course:del
    };

    struct Quest {
        bool list = false;          // quest:list:read
        bool read = false;          // quest:read
        bool update = false;        // quest:update
        bool create = false;        // quest:create
        bool del = false;           // quest:del
    };

    struct Test {
        bool questDel = false;      // test:quest:del
        bool questAdd = false;      // test:quest:add
        bool questUpdate = false;   // test:quest:update
        bool answerRead = false;    // test:answer:read
    };

    struct Attempt {
        bool create = false;        // attempt:create
        bool update = false;        // attempt:update
        bool finish = false;        // attempt:finish
        bool read = false;          // attempt:read
    };

    struct Answer {
        bool read = false;          // answer:read
        bool update = false;        // answer:update
        bool del = false;           // answer:del
    };
};


enum class Action {
    Error401,           //токен неверный
    Error403            //недостаточно прав для действия
};



Action CheckToken(SOCKET clientSocket, char* message, int msgSize);