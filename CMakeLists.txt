cmake_minimum_required(VERSION 3.10)
project(ChatServer)

set(CMAKE_CXX_STANDARD 17)

# Set Windows-specific definitions
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

add_executable(chat_server server.cpp)
add_executable(chat_client client.cpp)
add_executable(chat_client_enhanced chat_client_enhanced.cpp)
add_executable(chat_server_enhanced chat_server_enhanced.cpp)
add_executable(chat_client_advanced chat_client_advanced.cpp)

if (WIN32)
    target_link_libraries(chat_server ws2_32)
    target_link_libraries(chat_client ws2_32)
    target_link_libraries(chat_client_enhanced ws2_32)
    target_link_libraries(chat_server_enhanced ws2_32)
    target_link_libraries(chat_client_advanced ws2_32)
endif()
