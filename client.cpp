#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#pragma comment(lib, "ws2_32.lib")

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // Create socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    // Server address
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080); // match server port
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // Connect to server
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server. Type a message: ";
    std::string msg;
    std::getline(std::cin, msg);

    // Send message
    send(clientSocket, msg.c_str(), msg.size(), 0);

    // Receive echo
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << "Server echoed: " << buffer << "\n";
    }

    // Cleanup
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
