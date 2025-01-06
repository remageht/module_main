#pragma once
#include "CheckToken.h"
#include "SimpleFunctions.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include <winsock2.h>
#include <string>
#include <vector>
#include <thread>

void handleClient(SOCKET clientSocket);

