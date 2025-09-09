#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: ./client <server-ip> <port>\n";
        return 1;
    }

    const char* server_ip = argv[1];
    int port = std::stoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return 1;
    }

    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect");
        close(sock);
        return 1;
    }

    std::cout << "Connected to server. Type messages and press Enter.\n";
    std::string line;
    char buffer[1024];

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        ssize_t sent = send(sock, line.c_str(), (ssize_t)line.size(), 0);
        if (sent == -1) {
            perror("send");
            break;
        }

        ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            std::cout << "Disconnected from server\n";
            break;
        }
        buffer[n] = '\0';
        std::cout << "Echo from server: " << buffer << std::endl;
    }

    close(sock);
    return 0;
}