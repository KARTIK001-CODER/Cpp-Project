#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include <windows.h>
#include <winsock.h>
#pragma comment(lib, "ws2_32.lib")

class ChatClient {
private:
    SOCKET clientSocket;
    std::atomic<bool> running;
    std::string username;
    std::string room;
    
public:
    ChatClient() : clientSocket(INVALID_SOCKET), running(false) {}
    
    ~ChatClient() {
        disconnect();
    }
    
    bool initialize() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed\n";
            return false;
        }
        return true;
    }
    
    void cleanup() {
        WSACleanup();
    }
    
    void getUserInput() {
        std::cout << "=== Multi-Client Chat Client ===\n";
        std::cout << "Enter your username: ";
        std::getline(std::cin, username);
        
        std::cout << "Enter room name: ";
        std::getline(std::cin, room);
        
        if (username.empty()) {
            username = "Anonymous";
        }
        if (room.empty()) {
            room = "General";
        }
        
        std::cout << "Username: " << username << "\n";
        std::cout << "Room: " << room << "\n";
        std::cout << "Connecting to server...\n";
    }
    
    bool connectToServer() {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed\n";
            return false;
        }
        
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(8080);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Connection failed. Make sure the server is running on 127.0.0.1:8080\n";
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
            return false;
        }
        
        std::cout << "Connected to server successfully!\n";
        return true;
    }
    
    void sendMessage(const std::string& message) {
        if (clientSocket != INVALID_SOCKET) {
            std::string fullMessage = "[" + username + " in " + room + "]: " + message;
            send(clientSocket, fullMessage.c_str(), fullMessage.length(), 0);
        }
    }
    
    void receiverThread() {
        char buffer[1024];
        int bytesReceived;
        
        while (running && clientSocket != INVALID_SOCKET) {
            bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                std::cout << "\n" << buffer << "\n";
                std::cout << "[" << username << "]: ";
                std::cout.flush();
            }
            else if (bytesReceived == 0) {
                std::cout << "\nServer disconnected.\n";
                break;
            }
            else {
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK) {
                    std::cout << "\nConnection lost. Error: " << error << "\n";
                    break;
                }
            }
        }
        
        running = false;
    }
    
    void run() {
        if (!initialize()) {
            return;
        }
        
        getUserInput();
        
        if (!connectToServer()) {
            cleanup();
            return;
        }
        
        // Send join message
        sendMessage("joined the chat");
        
        running = true;
        std::thread receiver(&ChatClient::receiverThread, this);
        
        std::cout << "\n=== Chat Started ===\n";
        std::cout << "Type your messages and press Enter to send.\n";
        std::cout << "Type 'quit' or 'exit' to leave the chat.\n\n";
        std::cout << "[" << username << "]: ";
        std::cout.flush();
        
        std::string message;
        while (running) {
            std::getline(std::cin, message);
            
            if (!running) break;
            
            if (message.empty()) {
                std::cout << "[" << username << "]: ";
                std::cout.flush();
                continue;
            }
            
            if (message == "quit" || message == "exit") {
                sendMessage("left the chat");
                running = false;
                break;
            }
            
            sendMessage(message);
            std::cout << "[" << username << "]: ";
            std::cout.flush();
        }
        
        if (receiver.joinable()) {
            receiver.join();
        }
        
        disconnect();
        cleanup();
    }
    
    void disconnect() {
        running = false;
        if (clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
        }
    }
};

int main() {
    // Set console to handle UTF-8 for better display
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    ChatClient client;
    client.run();
    
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    
    return 0;
}
