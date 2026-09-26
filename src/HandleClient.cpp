#include "HandleClient.h"

#include <cstring>
#include <sstream>

#include "CheckToken.h"
#include "Http.h"
#include "Logger.h"
#include "Proxy.h"
#include "Router.h"
#include "SimpleFunctions.h"

namespace {
using gateway::gwLog;
using gateway::HttpRequest;
using gateway::HttpResponse;

bool sendAll(SOCKET s, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        int chunk = static_cast<int>((len - sent) > 65536 ? 65536 : (len - sent));
        int n = ::send(s, data + sent, chunk, 0);
        if (n == SOCKET_ERROR || n == 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

void reply(SOCKET s, long long connId, const HttpResponse& r) {
    std::string wire = gateway::buildResponse(r);
    if (!sendAll(s, wire.data(), wire.size())) {
        gwLog(gateway::LogLevel::Debug, connId, "Send failed.");
    }
}

long long parseContentLength(const std::string& v, bool& ok) {
    ok = false;
    if (v.empty() || v.size() > 10) return 0;
    long long n = 0;
    for (char c : v) {
        if (c < '0' || c > '9') return 0;
        n = n * 10 + (c - '0');
        if (n > 64LL * 1024 * 1024) return 0;
    }
    ok = true;
    return n;
}

std::string bearerFromHeader(const std::string& authHeader) {
    const std::string scheme = "Bearer ";
    if (authHeader.compare(0, scheme.size(), scheme) != 0) return "";
    std::string t = authHeader.substr(scheme.size());
    // trim spaces/tabs
    size_t a = t.find_first_not_of(" \t");
    if (a == std::string::npos) return "";
    size_t b = t.find_last_not_of(" \t");
    return t.substr(a, b - a + 1);
}
}  // namespace

void handleClient(SOCKET clientSocket, const std::string& clientIp,
                  const gateway::Config& cfg, gateway::RateLimiter& limiter,
                  long long connId) {
    using gateway::LogLevel;
    gwLog(LogLevel::Info, connId, "New connection from " + clientIp);

    // ---- 1. Read request head (until \r\n\r\n), cap kMaxHeaderBytes.
    std::string head;
    head.reserve(4096);
    char buf[4096];
    bool headDone = false;
    while (head.size() < static_cast<size_t>(gateway::kMaxHeaderBytes)) {
        int n = ::recv(clientSocket, buf, sizeof(buf), 0);
        if (n == 0 || n == SOCKET_ERROR) {
            gwLog(LogLevel::Debug, connId, "Client disconnected while reading head.");
            close_socket(clientSocket);
            return;
        }
        head.append(buf, static_cast<size_t>(n));
        if (head.find("\r\n\r\n") != std::string::npos) {
            headDone = true;
            break;
        }
    }
    if (!headDone) {
        gwLog(LogLevel::Warn, connId, "Header too large, closing.");
        reply(clientSocket, connId,
              gateway::jsonError(413, "headers_too_large", "Request head is too large."));
        close_socket(clientSocket);
        return;
    }

    size_t eoh = head.find("\r\n\r\n");
    std::string headPart = head.substr(0, eoh);
    std::string leftover = head.substr(eoh + 4);

    HttpRequest req;
    if (!gateway::parseHttpRequest(headPart, req)) {
        gwLog(LogLevel::Warn, connId, "Malformed HTTP request.");
        reply(clientSocket, connId,
              gateway::jsonError(400, "bad_request", "Malformed HTTP request."));
        close_socket(clientSocket);
        return;
    }
    req.clientIp = clientIp;

    // ---- 2. Body (Content-Length only; chunked is not supported in v1).
    std::string te = req.header("transfer-encoding");
    if (!te.empty()) {
        reply(clientSocket, connId,
              gateway::jsonError(400, "bad_request", "Chunked transfer is not supported."));
        close_socket(clientSocket);
        return;
    }
    bool clOk = false;
    long long cl = parseContentLength(req.header("content-length"), clOk);
    if (!req.header("content-length").empty() && !clOk) {
        reply(clientSocket, connId,
              gateway::jsonError(400, "bad_request", "Bad Content-Length."));
        close_socket(clientSocket);
        return;
    }
    if (cl > cfg.maxBodyBytes) {
        gwLog(LogLevel::Warn, connId, "Body too large: " + std::to_string(cl));
        reply(clientSocket, connId,
              gateway::jsonError(413, "body_too_large", "Request body exceeds the limit."));
        close_socket(clientSocket);
        return;
    }
    req.body = leftover;
    while (static_cast<long long>(req.body.size()) < cl) {
        int n = ::recv(clientSocket, buf, sizeof(buf), 0);
        if (n == 0 || n == SOCKET_ERROR) {
            gwLog(LogLevel::Debug, connId, "Client disconnected while reading body.");
            close_socket(clientSocket);
            return;
        }
        req.body.append(buf, static_cast<size_t>(n));
        if (static_cast<long long>(req.body.size()) > cfg.maxBodyBytes) {
            reply(clientSocket, connId,
                  gateway::jsonError(413, "body_too_large", "Request body exceeds the limit."));
            close_socket(clientSocket);
            return;
        }
    }
    if (static_cast<long long>(req.body.size()) > cl)
        req.body.resize(static_cast<size_t>(cl));

    gwLog(LogLevel::Info, connId, req.method + " " + req.target);

    // ---- 3. Rate limit (per client IP).
    if (!limiter.allow(clientIp)) {
        gwLog(LogLevel::Warn, connId, "Rate limited: " + clientIp);
        reply(clientSocket, connId,
              gateway::jsonError(429, "rate_limited", "Too many requests, slow down."));
        close_socket(clientSocket);
        return;
    }

    // ---- 4. Route.
    gateway::RouteResult route = gateway::matchRoute(req.method, req.target, cfg);
    if (route.kind == gateway::RouteResult::kHealth) {
        std::ostringstream oss;
        oss << "{\"status\":\"ok\",\"version\":\"" << gateway::kGatewayVersion
            << "\",\"upstreams\":{";
        bool first = true;
        for (const auto& area : gateway::apiAreas()) {
            if (!first) oss << ",";
            first = false;
            oss << "\"" << area << "\":\"" << cfg.upstreamFor(area) << "\"";
        }
        oss << "}}";
        HttpResponse hr;
        hr.status = 200;
        hr.headers["content-type"] = "application/json";
        hr.body = oss.str();
        reply(clientSocket, connId, hr);
        close_socket(clientSocket);
        return;
    }
    if (route.kind == gateway::RouteResult::kNotFound) {
        reply(clientSocket, connId, gateway::jsonError(404, "not_found", "Unknown path."));
        close_socket(clientSocket);
        return;
    }
    if (route.kind == gateway::RouteResult::kMethodNotAllowed) {
        reply(clientSocket, connId,
              gateway::jsonError(405, "method_not_allowed", "Use GET/POST/PUT/PATCH/DELETE."));
        close_socket(clientSocket);
        return;
    }

    // ---- 5. Auth: Bearer JWT -> 401; scope -> 403.
    std::string token = bearerFromHeader(req.header("authorization"));
    if (token.empty() || token.size() > 8192) {
        reply(clientSocket, connId,
              gateway::jsonError(401, "unauthorized", "Missing bearer token."));
        close_socket(clientSocket);
        return;
    }
    AuthInfo auth = verifyJwt(token, cfg.jwtSecret, cfg.jwtIssuer);
    if (!auth.ok) {
        gwLog(LogLevel::Warn, connId, "Auth failed: " + auth.error);
        reply(clientSocket, connId, gateway::jsonError(401, "unauthorized", auth.error));
        close_socket(clientSocket);
        return;
    }
    gwLog(LogLevel::Debug, connId,
          "Auth ok sub=" + (auth.sub.empty() ? "-" : auth.sub) +
              " token=" + maskToken(token));
    if (!gateway::hasScope(auth, route.area, route.requiredScope)) {
        gwLog(LogLevel::Warn, connId, "Forbidden: need " + route.requiredScope);
        reply(clientSocket, connId,
              gateway::jsonError(403, "forbidden",
                                 "Missing required scope: " + route.requiredScope));
        close_socket(clientSocket);
        return;
    }

    // ---- 6. Proxy to upstream module.
    if (route.upstreamBase.empty()) {
        reply(clientSocket, connId,
              gateway::jsonError(502, "bad_gateway",
                                 "Upstream not configured: " + route.area));
        close_socket(clientSocket);
        return;
    }
    gateway::ProxyResult pr =
        gateway::forwardRequest(route.upstreamBase, req, route.forwardPath, auth,
                                cfg.upstreamTimeoutMs);
    if (!pr.ok) {
        gwLog(LogLevel::Warn, connId, "Upstream error: " + pr.error);
        if (pr.timedOut) {
            reply(clientSocket, connId,
                  gateway::jsonError(504, "upstream_timeout", "Upstream module timed out."));
        } else {
            reply(clientSocket, connId,
                  gateway::jsonError(502, "bad_gateway", "Upstream error: " + pr.error));
        }
        close_socket(clientSocket);
        return;
    }
    gwLog(LogLevel::Info, connId, "Upstream " + route.area + " -> " + std::to_string(pr.status));
    HttpResponse out;
    out.status = pr.status;
    out.headers["content-type"] = pr.contentType;
    out.body = pr.body;
    reply(clientSocket, connId, out);
    close_socket(clientSocket);
}
