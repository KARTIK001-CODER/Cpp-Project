#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include <algorithm>
#include <map>
#include <sstream>
#include <ctime>
#include "windows_sockets.h"

struct Client {
    SOCKET socket;
    std::string username;
    std::string room;
    bool connected;
    
    Client(SOCKET s) : socket(s), connected(true) {}
};

struct Room {
    std::vector<std::string> messageHistory;
    std::vector<SOCKET> clients;
    std::mutex roomMutex;
};

class ChatServer {
private:
    SOCKET serverSocket;
    std::vector<Client> clients;
    std::mutex clientsMutex;
    std::map<std::string, Room> rooms;
    std::mutex roomsMutex;
    bool running;
    
    std::string getCurrentTime() {
        time_t now = time(0);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        return std::string(buffer);
    }
    
    void addToRoom(const std::string& roomName, SOCKET clientSocket) {
        std::lock_guard<std::mutex> lock(roomsMutex);
        rooms[roomName].clients.push_back(clientSocket);
    }
    
    void removeFromRoom(const std::string& roomName, SOCKET clientSocket) {
        std::lock_guard<std::mutex> lock(roomsMutex);
        auto& room = rooms[roomName];
        room.clients.erase(
            std::remove(room.clients.begin(), room.clients.end(), clientSocket),
            room.clients.end()
        );
    }
    
    void addMessageToRoom(const std::string& roomName, const std::string& message) {
        std::lock_guard<std::mutex> lock(roomsMutex);
        rooms[roomName].messageHistory.push_back(message);
        
        // Keep only last 100 messages per room
        if (rooms[roomName].messageHistory.size() > 100) {
            rooms[roomName].messageHistory.erase(rooms[roomName].messageHistory.begin());
        }
    }
    
    void sendMessageToRoom(const std::string& roomName, const std::string& message, SOCKET sender = INVALID_SOCKET) {
        std::lock_guard<std::mutex> lock(roomsMutex);
        auto it = rooms.find(roomName);
        if (it != rooms.end()) {
            for (SOCKET client : it->second.clients) {
                if (client != sender && client != INVALID_SOCKET) {
                    send(client, message.c_str(), message.length(), 0);
                }
            }
        }
    }
    
    void sendMessageHistory(SOCKET clientSocket, const std::string& roomName) {
        std::lock_guard<std::mutex> lock(roomsMutex);
        auto it = rooms.find(roomName);
        if (it != rooms.end()) {
            std::string historyMsg = "\n=== Room History ===\n";
            for (const auto& msg : it->second.messageHistory) {
                historyMsg += msg + "\n";
            }
            historyMsg += "=== End History ===\n";
            send(clientSocket, historyMsg.c_str(), historyMsg.length(), 0);
        }
    }
    
    std::vector<std::string> getUsersInRoom(const std::string& roomName) {
        std::lock_guard<std::mutex> lock(clientsMutex);
        std::vector<std::string> users;
        
        for (const auto& client : clients) {
            if (client.connected && client.room == roomName) {
                users.push_back(client.username);
            }
        }
        return users;
    }
    
    bool handleCommand(SOCKET clientSocket, const std::string& command, const std::string& username, const std::string& room) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "/list") {
            auto users = getUsersInRoom(room);
            std::string userList = "\n=== Users in room '" + room + "' ===\n";
            for (const auto& user : users) {
                userList += "- " + user + "\n";
            }
            userList += "Total: " + std::to_string(users.size()) + " users\n";
            send(clientSocket, userList.c_str(), userList.length(), 0);
            return true;
        }
        else if (cmd == "/rooms") {
            std::lock_guard<std::mutex> lock(roomsMutex);
            std::string roomList = "\n=== Available Rooms ===\n";
            for (const auto& roomPair : rooms) {
                roomList += "- " + roomPair.first + " (" + std::to_string(roomPair.second.clients.size()) + " users)\n";
            }
            roomList += "Total: " + std::to_string(rooms.size()) + " rooms\n";
            send(clientSocket, roomList.c_str(), roomList.length(), 0);
            return true;
        }
        else if (cmd == "/help") {
            std::string help = "\n=== Available Commands ===\n";
            help += "/list - Show users in current room\n";
            help += "/rooms - Show all available rooms\n";
            help += "/quit - Leave the chat\n";
            help += "/help - Show this help message\n";
            send(clientSocket, help.c_str(), help.length(), 0);
            return true;
        }
        
        return false;
    }
    
public:
    ChatServer() : serverSocket(INVALID_SOCKET), running(false) {}
    
    ~ChatServer() {
        stop();
    }
    
    bool initialize() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed\n";
            return false;
        }
        
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed\n";
            WSACleanup();
            return false;
        }
        
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(8080);
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        
        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed\n";
            closesocket(serverSocket);
            WSACleanup();
            return false;
        }
        
        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed\n";
            closesocket(serverSocket);
            WSACleanup();
            return false;
        }
        
        return true;
    }
    
    void handleClient(SOCKET clientSocket) {
        char buffer[1024];
        int bytesReceived;
        std::string username, room;
        bool userInfoReceived = false;
        
        // Send welcome message
        std::string welcome = "Welcome to the chat server!\n";
        welcome += "Please send your username and room in format: USERNAME|ROOM\n";
        send(clientSocket, welcome.c_str(), welcome.length(), 0);
        
        while (running) {
            bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                std::string message(buffer);
                
                if (!userInfoReceived) {
                    // Parse username and room
                    size_t pos = message.find('|');
                    if (pos != std::string::npos) {
                        username = message.substr(0, pos);
                        room = message.substr(pos + 1);
                        
                        // Remove newline characters
                        username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
                        username.erase(std::remove(username.begin(), username.end(), '\r'), username.end());
                        room.erase(std::remove(room.begin(), room.end(), '\n'), room.end());
                        room.erase(std::remove(room.begin(), room.end(), '\r'), room.end());
                        
                        if (username.empty()) username = "Anonymous";
                        if (room.empty()) room = "General";
                        
                        // Add client to our list
                        {
                            std::lock_guard<std::mutex> lock(clientsMutex);
                            clients.emplace_back(clientSocket);
                            clients.back().username = username;
                            clients.back().room = room;
                        }
                        
                        addToRoom(room, clientSocket);
                        userInfoReceived = true;
                        
                        // Send room history
                        sendMessageHistory(clientSocket, room);
                        
                        // Notify others in room
                        std::string joinMsg = "[" + getCurrentTime() + "] " + username + " joined the room '" + room + "'";
                        addMessageToRoom(room, joinMsg);
                        sendMessageToRoom(room, joinMsg, clientSocket);
                        
                        std::cout << "Client " << username << " joined room " << room << std::endl;
                    }
                }
                else {
                    // Handle regular messages and commands
                    if (message[0] == '/') {
                        if (!handleCommand(clientSocket, message, username, room)) {
                            std::string errorMsg = "Unknown command. Type /help for available commands.";
                            send(clientSocket, errorMsg.c_str(), errorMsg.length(), 0);
                        }
                    }
                    else {
                        // Regular message
                        std::string fullMessage = "[" + getCurrentTime() + "] " + username + ": " + message;
                        addMessageToRoom(room, fullMessage);
                        sendMessageToRoom(room, fullMessage, clientSocket);
                        std::cout << "[" << room << "] " << fullMessage << std::endl;
                    }
                }
            }
            else if (bytesReceived == 0) {
                std::cout << "Client disconnected\n";
                break;
            }
            else {
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK) {
                    std::cout << "Client error: " << error << std::endl;
                    break;
                }
            }
        }
        
        // Cleanup
        if (userInfoReceived) {
            removeFromRoom(room, clientSocket);
            
            // Notify others in room
            std::string leaveMsg = "[" + getCurrentTime() + "] " + username + " left the room";
            addMessageToRoom(room, leaveMsg);
            sendMessageToRoom(room, leaveMsg);
            
            // Remove from clients list
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                clients.erase(
                    std::remove_if(clients.begin(), clients.end(),
                        [clientSocket](const Client& c) { return c.socket == clientSocket; }),
                    clients.end()
                );
            }
        }
        
        closesocket(clientSocket);
    }
    
    void run() {
        if (!initialize()) {
            return;
        }
        
        running = true;
        std::cout << "Chat server listening on port 8080...\n";
        std::cout << "Press Ctrl+C to stop the server\n\n";
        
        while (running) {
            sockaddr_in clientAddr;
            int clientSize = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
            
            if (clientSocket != INVALID_SOCKET) {
                std::thread(&ChatServer::handleClient, this, clientSocket).detach();
            }
        }
    }
    
    void stop() {
        running = false;
        if (serverSocket != INVALID_SOCKET) {
            closesocket(serverSocket);
            serverSocket = INVALID_SOCKET;
        }
        
        // Close all client connections
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (auto& client : clients) {
                closesocket(client.socket);
            }
            clients.clear();
        }
        
        WSACleanup();
    }
};

int main() {
    ChatServer server;
    
    // Handle Ctrl+C gracefully
    SetConsoleCtrlHandler([](DWORD ctrlType) -> BOOL {
        if (ctrlType == CTRL_C_EVENT) {
            std::cout << "\nShutting down server...\n";
            return TRUE;
        }
        return FALSE;
    }, TRUE);
    
    server.run();
    return 0;
}
