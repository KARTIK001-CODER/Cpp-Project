#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include <windows.h>
#include <winsock.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#pragma comment(lib, "ws2_32.lib")

class ChatClient {
private:
    SOCKET clientSocket;
    std::atomic<bool> running;
    std::string username;
    std::string room;
    
    std::string getCurrentTime() {
        time_t now = time(0);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        return std::string(buffer);
    }
    
    void displayMessage(const std::string& message) {
        std::cout << "\r" << std::string(80, ' ') << "\r"; // Clear current line
        std::cout << message << std::endl;
        std::cout << "[" << getCurrentTime() << "] [" << username << "]: ";
        std::cout.flush();
    }
    
    void displaySystemMessage(const std::string& message) {
        std::cout << "\r" << std::string(80, ' ') << "\r"; // Clear current line
        std::cout << "\033[33m" << message << "\033[0m" << std::endl; // Yellow color for system messages
        std::cout << "[" << getCurrentTime() << "] [" << username << "]: ";
        std::cout.flush();
    }
    
    void displayUserMessage(const std::string& message) {
        std::cout << "\r" << std::string(80, ' ') << "\r"; // Clear current line
        std::cout << message << std::endl;
        std::cout << "[" << getCurrentTime() << "] [" << username << "]: ";
        std::cout.flush();
    }
    
    void displayPrompt() {
        std::cout << "[" << getCurrentTime() << "] [" << username << "]: ";
        std::cout.flush();
    }
    
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
        std::cout << "=== Advanced Multi-Client Chat Client ===\n";
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
            send(clientSocket, message.c_str(), message.length(), 0);
        }
    }
    
    void sendUserInfo() {
        std::string userInfo = username + "|" + room;
        sendMessage(userInfo);
    }
    
    void receiverThread() {
        char buffer[1024];
        int bytesReceived;
        
        while (running && clientSocket != INVALID_SOCKET) {
            bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                std::string message(buffer);
                
                // Check if it's a system message (contains "===" or specific keywords)
                if (message.find("===") != std::string::npos || 
                    message.find("joined") != std::string::npos || 
                    message.find("left") != std::string::npos ||
                    message.find("Users in room") != std::string::npos ||
                    message.find("Available Rooms") != std::string::npos ||
                    message.find("Available Commands") != std::string::npos ||
                    message.find("Unknown command") != std::string::npos) {
                    displaySystemMessage(message);
                }
                else {
                    displayUserMessage(message);
                }
            }
            else if (bytesReceived == 0) {
                displaySystemMessage("Server disconnected.");
                break;
            }
            else {
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK) {
                    displaySystemMessage("Connection lost. Error: " + std::to_string(error));
                    break;
                }
            }
        }
        
        running = false;
    }
    
    void showHelp() {
        std::cout << "\n=== Available Commands ===\n";
        std::cout << "/list - Show users in current room\n";
        std::cout << "/rooms - Show all available rooms\n";
        std::cout << "/quit - Leave the chat\n";
        std::cout << "/help - Show this help message\n";
        std::cout << "========================\n\n";
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
        
        // Send user info to server
        sendUserInfo();
        
        running = true;
        std::thread receiver(&ChatClient::receiverThread, this);
        
        std::cout << "\n=== Chat Started ===\n";
        std::cout << "Type your messages and press Enter to send.\n";
        std::cout << "Type /help for available commands.\n";
        std::cout << "Type /quit to exit.\n\n";
        
        showHelp();
        displayPrompt();
        
        std::string message;
        while (running) {
            std::getline(std::cin, message);
            
            if (!running) break;
            
            if (message.empty()) {
                displayPrompt();
                continue;
            }
            
            if (message == "/quit" || message == "/exit") {
                running = false;
                break;
            }
            else if (message == "/help") {
                showHelp();
                displayPrompt();
                continue;
            }
            else if (message[0] == '/') {
                // Send command to server
                sendMessage(message);
                displayPrompt();
            }
            else {
                // Regular message
                sendMessage(message);
                displayPrompt();
            }
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
    
    // Enable ANSI color codes for Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    
    ChatClient client;
    client.run();
    
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    
    return 0;
}
