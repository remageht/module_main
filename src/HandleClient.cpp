#include "HandleClient.h"

void handleClient(SOCKET clientSocket) {
    // новое подключение к серверу
    std::cout << "New connection on socket " << clientSocket << std::endl;

    //  ожидается JWT токен

    const int msgSize = 1024;
    char* message = new char[msgSize];

    while (true)
    {
        int bytesReceived = recv(clientSocket, message, msgSize, NULL);

        if (bytesReceived == 0 || bytesReceived == -1) {
            beautyPrint(clientSocket, "Client has disconnected or a connection error has occurred.");
            break;
        }
        beautyPrint(clientSocket, "Message : " +  std::string(message));
    
        switch (CheckToken(clientSocket, message, msgSize))
        {
        case Action::Error401:
            beautyPrint(clientSocket, "Resource: Error401");
{
            const char* httpResponse = 
            "HTTP/1.1 401 Forbidden\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "Access Denied";
    
            beautyPrint(clientSocket, "Send code 401");
            send(clientSocket, httpResponse, strlen(httpResponse), 0);
}

            closesocket(clientSocket);
            beautyPrint(clientSocket, "Socket closed.");
            break;


        case Action::Error403:
            beautyPrint(clientSocket, "Resource: Error403");
{
            const char* httpResponse = 
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "Access Denied";
    
            beautyPrint(clientSocket, "Send code 403");
            send(clientSocket, httpResponse, strlen(httpResponse), 0);
}

            closesocket(clientSocket);
            beautyPrint(clientSocket, "Socket closed.");
            break;
        }
    }    
}
