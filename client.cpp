#include <iostream>
#include <thread>
#include <string>
#include "windows_sockets.h"
#pragma comment(lib, "ws2_32.lib")

void receiveMessages(SOCKET serverSocket) {
    char buffer[1024];
    int bytesReceived;

    while ((bytesReceived = recv(serverSocket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << buffer << std::endl;
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server. You can start typing messages...\n";

    std::thread receiver(receiveMessages, clientSocket);
    receiver.detach();

    std::string msg;
    while (true) {
        std::getline(std::cin, msg);
        if (msg.empty()) continue;
        send(clientSocket, msg.c_str(), msg.size(), 0);
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
