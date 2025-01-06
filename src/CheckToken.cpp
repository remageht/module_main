#include "CheckToken.h"

Action CheckToken(SOCKET clientSocket, char* message, int msgSize)
{
    std::string secret = "key";

    beautyPrint(clientSocket, "Find token...");
    std::string token = findToken(message);

    if (token == "")
    {
        beautyPrint(clientSocket, "Token not found.");
        return Action::Error401;
    }
    
    beautyPrint(clientSocket, "Token: " + token);

    return Action::Error403;
}
