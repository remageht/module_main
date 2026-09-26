#include "Proxy.h"

#include <cctype>
#include <cstdio>
#include <vector>

#include "Config.h"
#include "NetCompat.h"

namespace gateway {
namespace {

std::string sanitizeHeaderValue(const std::string& v) {
    std::string o;
    o.reserve(v.size());
    for (char c : v) {
        if (c == '\r' || c == '\n') o += ' ';
        else o += c;
    }
    return o;
}

std::string joinCsv(const std::vector<std::string>& v) {
    std::string o;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) o += ',';
        o += sanitizeHeaderValue(v[i]);
    }
    return o;
}

struct UpstreamAddr {
    bool ok = false;
    std::string host;
    int port = 80;
    std::string prefix;  // "" or "/v1"
    std::string error;
};

UpstreamAddr parseUpstream(const std::string& base) {
    UpstreamAddr a;
    const std::string scheme = "http://";
    if (base.compare(0, scheme.size(), scheme) != 0) {
        a.error = "unsupported scheme (want http://)";
        return a;
    }
    std::string rest = base.substr(scheme.size());
    size_t slash = rest.find('/');
    std::string hostport = (slash == std::string::npos) ? rest : rest.substr(0, slash);
    a.prefix = (slash == std::string::npos) ? "" : rest.substr(slash);
    // Trim trailing '/' from prefix ("/v1/" -> "/v1").
    while (a.prefix.size() > 1 && a.prefix.back() == '/') a.prefix.pop_back();
    size_t colon = hostport.rfind(':');
    if (colon != std::string::npos) {
        a.host = hostport.substr(0, colon);
        try {
            a.port = std::stoi(hostport.substr(colon + 1));
        } catch (...) {
            a.error = "bad port";
            return a;
        }
        if (a.port <= 0 || a.port > 65535) {
            a.error = "bad port";
            return a;
        }
    } else {
        a.host = hostport;
        a.port = 80;
    }
    if (a.host.empty()) {
        a.error = "empty host";
        return a;
    }
    a.ok = true;
    return a;
}

bool connectWithTimeout(SOCKET s, const struct sockaddr* addr, int addrLen, int timeoutMs,
                        bool& timedOut) {
    timedOut = false;
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(s, FIONBIO, &mode) != 0) return false;
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) return false;
    if (fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0) return false;
#endif
    int rc = ::connect(s, addr, addrLen);
    if (rc == 0) {
#ifdef _WIN32
        mode = 0;
        ioctlsocket(s, FIONBIO, &mode);
#else
        fcntl(s, F_SETFL, flags);
#endif
        return true;
    }
#ifdef _WIN32
    if (WSAGetLastError() != WSAEWOULDBLOCK) return false;
#else
    if (errno != EINPROGRESS) return false;
#endif
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(s, &wfds);
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = static_cast<decltype(tv.tv_usec)>((timeoutMs % 1000) * 1000);
    rc = select(static_cast<int>(s) + 1, nullptr, &wfds, nullptr, &tv);
    if (rc == 0) {
        timedOut = true;
        return false;
    }
    if (rc < 0) return false;
    int soErr = 0;
#ifdef _WIN32
    int soLen = sizeof(soErr);
#else
    socklen_t soLen = sizeof(soErr);
#endif
    if (getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soErr), &soLen) != 0)
        return false;
    if (soErr != 0) return false;
#ifdef _WIN32
    mode = 0;
    ioctlsocket(s, FIONBIO, &mode);
#else
    fcntl(s, F_SETFL, flags);
#endif
    return true;
}

bool sendAll(SOCKET s, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        int chunk = static_cast<int>((len - sent) > 65536 ? 65536 : (len - sent));
        int n = ::send(s, data + sent, chunk, 0);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

}  // namespace

ProxyResult forwardRequest(const std::string& upstreamBase, const HttpRequest& req,
                           const std::string& forwardPath, const AuthInfo& auth,
                           int timeoutMs) {
    ProxyResult pr;
    UpstreamAddr up = parseUpstream(upstreamBase);
    if (!up.ok) {
        pr.error = "bad upstream url: " + up.error;
        return pr;
    }

    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* list = nullptr;
    char portStr[16];
    snprintf(portStr, sizeof(portStr), "%d", up.port);
    if (getaddrinfo(up.host.c_str(), portStr, &hints, &list) != 0 || list == nullptr) {
        pr.error = "dns resolve failed";
        if (list) freeaddrinfo(list);
        return pr;
    }

    SOCKET s = kInvalidSock;
    bool connected = false;
    bool timedOut = false;
    for (struct addrinfo* ai = list; ai != nullptr; ai = ai->ai_next) {
        s = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (s == kInvalidSock) continue;
        if (connectWithTimeout(s, ai->ai_addr, static_cast<int>(ai->ai_addrlen), timeoutMs,
                               timedOut)) {
            connected = true;
            break;
        }
        bool wasTimeout = timedOut;
        close_socket(s);
        s = kInvalidSock;
        if (wasTimeout) break;  // no point trying other addresses after a timeout
    }
    freeaddrinfo(list);
    if (!connected) {
        pr.error = timedOut ? "connect timeout" : "connect failed";
        pr.timedOut = timedOut;
        return pr;
    }
    set_recv_timeout(s, timeoutMs);
#ifdef _WIN32
    DWORD sndMs = static_cast<DWORD>(timeoutMs);
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&sndMs), sizeof(sndMs));
#else
    struct timeval stv;
    stv.tv_sec = timeoutMs / 1000;
    stv.tv_usec = static_cast<suseconds_t>((timeoutMs % 1000) * 1000);
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &stv, sizeof(stv));
#endif

    std::string path = up.prefix + forwardPath;
    if (path.empty() || path[0] != '/') path = "/" + path;

    std::string wire;
    wire.reserve(req.body.size() + 512);
    wire += req.method + " " + path + " HTTP/1.1\r\n";
    wire += "host: " + sanitizeHeaderValue(up.host) + "\r\n";
    std::string ctype = req.header("content-type");
    if (!ctype.empty()) wire += "content-type: " + sanitizeHeaderValue(ctype) + "\r\n";
    wire += "content-length: " + std::to_string(req.body.size()) + "\r\n";
    wire += "x-forwarded-for: " + sanitizeHeaderValue(req.clientIp) + "\r\n";
    if (!auth.sub.empty()) wire += "x-auth-sub: " + sanitizeHeaderValue(auth.sub) + "\r\n";
    if (!auth.roles.empty()) wire += "x-auth-roles: " + joinCsv(auth.roles) + "\r\n";
    if (!auth.permissions.empty())
        wire += "x-auth-scopes: " + joinCsv(auth.permissions) + "\r\n";
    wire += "connection: close\r\n\r\n";
    wire += req.body;

    bool sentOk = sendAll(s, wire.data(), wire.size());

    std::string raw;
    raw.reserve(8192);
    bool recvFail = false;
    if (sentOk) {
        char buf[8192];
        while (raw.size() < static_cast<size_t>(kMaxUpstreamBytes)) {
            int n = ::recv(s, buf, sizeof(buf), 0);
            if (n == 0) break;  // peer closed -> complete
            if (n < 0) {
                // Timeout vs hard error: a partial head is still a protocol failure.
                recvFail = true;
                break;
            }
            raw.append(buf, static_cast<size_t>(n));
        }
        if (raw.size() >= static_cast<size_t>(kMaxUpstreamBytes)) {
            close_socket(s);
            pr.error = "upstream response too large";
            return pr;
        }
    }
    close_socket(s);

    if (!sentOk || recvFail || raw.empty()) {
        pr.error = "upstream io failed";
        return pr;
    }
    size_t eoh = raw.find("\r\n\r\n");
    if (eoh == std::string::npos) {
        pr.error = "bad upstream response";
        return pr;
    }
    std::string head = raw.substr(0, eoh);
    size_t eol = head.find("\r\n");
    std::string statusLine = (eol == std::string::npos) ? head : head.substr(0, eol);
    // Expected: HTTP/1.x SP CODE SP ...
    int code = 0;
    {
        size_t sp1 = statusLine.find(' ');
        if (sp1 != std::string::npos) {
            size_t sp2 = statusLine.find(' ', sp1 + 1);
            std::string codeStr = statusLine.substr(sp1 + 1, sp2 == std::string::npos
                                                                 ? std::string::npos
                                                                 : sp2 - sp1 - 1);
            try {
                code = std::stoi(codeStr);
            } catch (...) {
                code = 0;
            }
        }
    }
    if (code < 100 || code > 599) {
        pr.error = "bad upstream status";
        return pr;
    }
    // Upstream content-type (case-insensitive scan of head lines).
    std::string lower = head;
    for (auto& c : lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    std::string upType;
    const std::string key = "\r\ncontent-type:";
    size_t pos = lower.find(key);
    if (pos != std::string::npos) {
        size_t vs = pos + key.size();
        size_t ve = lower.find("\r\n", vs);
        std::string val = head.substr(vs, ve == std::string::npos ? std::string::npos : ve - vs);
        // trim spaces/tabs
        size_t a = val.find_first_not_of(" \t");
        size_t b = val.find_last_not_of(" \t");
        if (a != std::string::npos) upType = val.substr(a, b - a + 1);
    }

    pr.ok = true;
    pr.status = code;
    pr.contentType = upType.empty() ? "application/octet-stream" : upType;
    pr.body = raw.substr(eoh + 4);
    return pr;
}

}  // namespace gateway
