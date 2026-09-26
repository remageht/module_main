#pragma once
// Cross-platform socket compat: Winsock2 on Windows, BSD sockets on Linux/macOS.
// All app code includes THIS header instead of <winsock2.h> directly.

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>

typedef int socklen_t;
constexpr SOCKET kInvalidSock = INVALID_SOCKET;

inline void close_socket(SOCKET s) {
    if (s != INVALID_SOCKET) closesocket(s);
}
inline bool net_init() {
    WSADATA d;
    return WSAStartup(MAKEWORD(2, 2), &d) == 0;
}
inline void net_cleanup() { WSACleanup(); }
inline int net_last_error() { return WSAGetLastError(); }
inline bool set_recv_timeout(SOCKET s, int ms) {
    DWORD t = static_cast<DWORD>(ms);
    return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,
                      reinterpret_cast<const char*>(&t), sizeof(t)) == 0;
}
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>

typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define closesocket(s) ::close(s)
#define WSAGetLastError() (errno)
typedef unsigned long DWORD;
struct WSADATA {};
#define MAKEWORD(a, b) (0)
inline int WSAStartup(int, void*) { return 0; }
inline void WSACleanup() {}

inline void close_socket(SOCKET s) {
    if (s != INVALID_SOCKET) ::close(s);
}
inline bool net_init() { return true; }
inline void net_cleanup() {}
inline int net_last_error() { return errno; }
inline bool set_recv_timeout(SOCKET s, int ms) {
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = static_cast<suseconds_t>((ms % 1000) * 1000);
    return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
}
#endif
