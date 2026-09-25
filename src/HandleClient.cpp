#include "HandleClient.h"

#include <cstring>

void handleClient(SOCKET clientSocket) {
    // новое подключение к серверу
    std::cout << "New connection on socket " << clientSocket << std::endl;

    //  ожидается JWT токен

    const int msgSize = 8192;
    std::vector<char> message(static_cast<size_t>(msgSize) + 1, 0);

    while (true)
    {
        int bytesReceived = recv(clientSocket, message.data(), msgSize, 0);

        if (bytesReceived == 0 || bytesReceived == SOCKET_ERROR) {
            beautyPrint(clientSocket, "Client has disconnected or a connection error has occurred.");
            break;
        }
        // recv не терминирует нулем — строим строку строго по bytesReceived.
        message[static_cast<size_t>(bytesReceived)] = '\0';
        std::string request(message.data(), static_cast<size_t>(bytesReceived));
        beautyPrint(clientSocket, "Message received, bytes=" + std::to_string(bytesReceived));
    
        switch (CheckToken(clientSocket, message.data(), msgSize))
        {
        case Action::Error401:
            beautyPrint(clientSocket, "Resource: Error401");
{
            const char* httpResponse = 
            "HTTP/1.1 401 Unauthorized\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Access Denied";
    
            beautyPrint(clientSocket, "Send code 401");
            if (send(clientSocket, httpResponse, (int)strlen(httpResponse), 0) == SOCKET_ERROR) {
                beautyPrint(clientSocket, "Send failed.");
            }
}

            closesocket(clientSocket);
            beautyPrint(clientSocket, "Socket closed.");
            return;


        case Action::Error403:
            beautyPrint(clientSocket, "Resource: Error403");
{
            const char* httpResponse = 
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Access Denied";
    
            beautyPrint(clientSocket, "Send code 403");
            if (send(clientSocket, httpResponse, (int)strlen(httpResponse), 0) == SOCKET_ERROR) {
                beautyPrint(clientSocket, "Send failed.");
            }
}

            closesocket(clientSocket);
            beautyPrint(clientSocket, "Socket closed.");
            return;
        }
    }

    closesocket(clientSocket);
}
