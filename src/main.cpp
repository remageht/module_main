// module_main v1: HTTP/JSON API gateway.
// Listens on BIND:PORT, verifies JWT, routes /api/<area>/... to upstream
// modules. See docs/API_CONTRACT.md. All config comes from environment.

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include "Config.h"
#include "HandleClient.h"
#include "Logger.h"
#include "NetCompat.h"
#include "Router.h"
#include "getAddress.h"

namespace {
volatile std::sig_atomic_t g_running = 1;
void onSignal(int) { g_running = 0; }

std::string peerIp(SOCKET s) {
    sockaddr_in addr = {};
    socklen_t len = sizeof(addr);
    if (getpeername(s, reinterpret_cast<sockaddr*>(&addr), &len) != 0) return "unknown";
    char buf[INET_ADDRSTRLEN] = {0};
    if (inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf)) == nullptr) return "unknown";
    return std::string(buf);
}

bool waitReadable(SOCKET listener) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(listener, &rfds);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500 * 1000;  // 500 ms poll so Ctrl+C is noticed promptly
#ifdef _WIN32
    int rc = select(0, &rfds, nullptr, nullptr, &tv);
#else
    int rc = select(listener + 1, &rfds, nullptr, nullptr, &tv);
#endif
    return rc > 0;
}
}  // namespace

int main() {
    using gateway::LogLevel;

    // ---- 1. Config (fail fast with a clear error, never start half-configured).
    gateway::ConfigResult cfgRes = gateway::loadConfig();
    if (!cfgRes.ok) {
        std::cerr << "Config error: " << cfgRes.error << std::endl;
        return 1;
    }
    const gateway::Config& cfg = cfgRes.cfg;
    gateway::setLogLevel(cfg.logLevel);

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    // ---- 2. Network init.
    if (!net_init()) {
        std::cerr << "Network init failed." << std::endl;
        return 1;
    }

    // ---- 3. Listener on BIND:PORT (IPv4).
    SOCKET listener = kInvalidSock;
    {
        struct addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        struct addrinfo* list = nullptr;
        std::string portStr = std::to_string(cfg.port);
        if (getaddrinfo(cfg.bind.c_str(), portStr.c_str(), &hints, &list) != 0 ||
            list == nullptr) {
            std::cerr << "Cannot resolve bind address '" << cfg.bind << "'." << std::endl;
            net_cleanup();
            return 1;
        }
        for (struct addrinfo* ai = list; ai != nullptr; ai = ai->ai_next) {
            SOCKET s = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if (s == kInvalidSock) continue;
#ifdef _WIN32
            const char reuse = 1;
#else
            int reuse = 1;
#endif
            setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
            if (bind(s, ai->ai_addr, static_cast<int>(ai->ai_addrlen)) == 0 &&
                listen(s, SOMAXCONN) == 0) {
                listener = s;
                break;
            }
            gateway::gwLog(LogLevel::Debug, -1,
                           "bind/listen failed: " + std::to_string(net_last_error()));
            close_socket(s);
        }
        freeaddrinfo(list);
    }
    if (listener == kInvalidSock) {
        std::cerr << "Cannot listen on " << cfg.bind << ":" << cfg.port
                  << " (err " << net_last_error() << "). Port busy?" << std::endl;
        net_cleanup();
        return 1;
    }

    // ---- 4. Banner (no secrets here).
    std::cout << "module_main gateway v" << gateway::kGatewayVersion << " started\n"
              << "Listen:    http://" << cfg.bind << ":" << cfg.port << "/\n"
              << "Health:    http://" << cfg.bind << ":" << cfg.port << "/health\n"
              << "Log level: " << cfg.logLevel << "\n"
              << "Upstreams:\n";
    for (const auto& area : gateway::apiAreas()) {
        std::string up = cfg.upstreamFor(area);
        std::cout << "  /api/" << area << "/* -> "
                  << (up.empty() ? "(not configured, 502 at runtime)" : up) << "\n";
    }
    std::cout << "Local IP in LAN: " << getLocalIPAddress() << std::endl << std::endl;

    // ---- 5. Accept loop with graceful shutdown.
    gateway::RateLimiter limiter(cfg.rateLimitPerMin);
    std::atomic<int> activeClients{0};
    std::atomic<long long> connSeq{0};

    while (g_running) {
        if (!waitReadable(listener)) continue;  // timeout or select error -> re-check flag
        sockaddr_in clientAddr = {};
        socklen_t clientLen = sizeof(clientAddr);
        SOCKET cs = accept(listener, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (cs == kInvalidSock) {
            if (g_running)
                gateway::gwLog(LogLevel::Warn, -1,
                               "accept failed: " + std::to_string(net_last_error()));
            continue;
        }
        if (activeClients.load() >= cfg.maxClients) {
            gateway::gwLog(LogLevel::Warn, -1, "Too many clients, connection rejected.");
            close_socket(cs);
            continue;
        }
        if (!set_recv_timeout(cs, cfg.recvTimeoutMs)) {
            gateway::gwLog(LogLevel::Warn, -1, "Cannot set recv timeout, closing.");
            close_socket(cs);
            continue;
        }
        std::string ip = peerIp(cs);
        long long id = connSeq.fetch_add(1) + 1;
        activeClients.fetch_add(1);
        std::thread([cs, ip, id, &cfg, &limiter, &activeClients]() {
            handleClient(cs, ip, cfg, limiter, id);
            activeClients.fetch_sub(1);
        }).detach();
    }

    std::cout << "\nShutting down, waiting for active connections..." << std::endl;
    close_socket(listener);
    for (int i = 0; i < 100 && activeClients.load() > 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    net_cleanup();
    std::cout << "Stopped." << std::endl;
    return 0;
}
