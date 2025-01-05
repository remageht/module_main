#include "HandleClient.h"

void handleClient(SOCKET clientSocket) {
    std::cout << "New connection on socket " << clientSocket << std::endl;

    const int msgSize = 1024;
    std::vector<char> message(msgSize);

    while (true) {
        int bytesReceived = recv(clientSocket, message.data(), msgSize, 0);

        if (bytesReceived <= 0) {
            beautyPrint(clientSocket, "Client has disconnected or a connection error has occurred.");
            break;
        }

        beautyPrint(clientSocket, "Message : " + std::string(message.begin(), message.end()));

        // можно добавить проверку на jwt токен
        beautyPrint(clientSocket, "Token check is skipped for now.");
        
        
        const char* httpResponse = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 7\r\n"
            "\r\n"
            "Success";
        
        beautyPrint(clientSocket, "Sending response code 200");
        send(clientSocket, httpResponse, strlen(httpResponse), 0);
        closesocket(clientSocket);
        beautyPrint(clientSocket, "Socket closed.");
        break;
    }
}
