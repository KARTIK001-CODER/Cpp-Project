cmake_minimum_required(VERSION 3.10)
project(ChatServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(chat_server server.cpp)

# Link Winsock2
if (WIN32)
    target_link_libraries(chat_server ws2_32)
endif()
