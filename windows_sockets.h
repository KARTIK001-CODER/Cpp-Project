#ifndef WINDOWS_SOCKETS_H
#define WINDOWS_SOCKETS_H

#ifdef _WIN32
#include <windows.h>
#include <winsock.h>

// Only define if not already defined
#ifndef SOCKET
typedef UINT_PTR SOCKET;
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET  (SOCKET)(~0)
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR            (-1)
#endif

// Only define constants if not already defined
#ifndef AF_INET
#define AF_INET         2
#endif

#ifndef SOCK_STREAM
#define SOCK_STREAM     1
#endif

#ifndef IPPROTO_TCP
#define IPPROTO_TCP     6
#endif

#ifndef INADDR_ANY
#define INADDR_ANY      ((unsigned long)0x00000000)
#endif

#ifndef SOMAXCONN
#define SOMAXCONN       0x7fffffff
#endif

#endif // _WIN32

#endif // WINDOWS_SOCKETS_H
