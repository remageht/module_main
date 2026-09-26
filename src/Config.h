#pragma once
// Production config: everything comes from environment (12-factor).
// JWT_SECRET is mandatory -> fail fast without it (see loadConfig).

#include <cstdlib>
#include <string>
#include <map>

namespace gateway {

constexpr const char* kGatewayVersion = "1.0.0";
constexpr int kMaxHeaderBytes = 16384;      // request head cap -> 413
constexpr int kMaxUpstreamBytes = 4 * 1024 * 1024;  // upstream body cap -> 502

inline std::string envStr(const char* name, const std::string& def) {
    const char* v = std::getenv(name);
    if (v == nullptr || v[0] == '\0') return def;
    return std::string(v);
}

inline int envInt(const char* name, int def, int minV, int maxV) {
    const char* v = std::getenv(name);
    if (v == nullptr || v[0] == '\0') return def;
    try {
        int n = std::stoi(v);
        if (n < minV || n > maxV) return def;
        return n;
    } catch (...) {
        return def;
    }
}

struct Config {
    std::string bind = "0.0.0.0";
    int port = 1111;
    std::string jwtSecret;
    std::string jwtIssuer;          // empty = issuer not enforced
    std::string logLevel = "info";

    int upstreamTimeoutMs = 5000;
    int recvTimeoutMs = 30000;
    int maxBodyBytes = 1024 * 1024;
    int maxClients = 128;
    int rateLimitPerMin = 100;

    // area -> upstream base URL ("http://host:port[/prefix]")
    std::map<std::string, std::string> upstreams;

    // Resolve upstream for an area, with fallback: quests/attempts/answers -> tests.
    std::string upstreamFor(const std::string& area) const {
        auto it = upstreams.find(area);
        if (it != upstreams.end() && !it->second.empty()) return it->second;
        if (area == "quests" || area == "attempts" || area == "answers") {
            auto t = upstreams.find("tests");
            if (t != upstreams.end()) return t->second;
        }
        return "";
    }
};

struct ConfigResult {
    bool ok = false;
    Config cfg;
    std::string error;  // set when !ok
};

inline ConfigResult loadConfig() {
    ConfigResult r;
    Config& c = r.cfg;

    c.bind = envStr("BIND", "0.0.0.0");
    c.port = envInt("PORT", 1111, 1, 65535);
    c.jwtSecret = envStr("JWT_SECRET", "");
    c.jwtIssuer = envStr("JWT_ISSUER", "");
    c.logLevel = envStr("LOG_LEVEL", "info");

    c.upstreamTimeoutMs = envInt("UPSTREAM_TIMEOUT_MS", 5000, 100, 120000);
    c.recvTimeoutMs = envInt("RECV_TIMEOUT_MS", 30000, 1000, 300000);
    c.maxBodyBytes = envInt("MAX_BODY_BYTES", 1024 * 1024, 1024, 16 * 1024 * 1024);
    c.maxClients = envInt("MAX_CLIENTS", 128, 1, 4096);
    c.rateLimitPerMin = envInt("RATE_LIMIT_PER_MIN", 100, 0, 100000);

    c.upstreams["users"] = envStr("MODULE_USERS_URL", "");
    c.upstreams["courses"] = envStr("MODULE_COURSES_URL", "");
    c.upstreams["quests"] = envStr("MODULE_QUESTS_URL", "");
    c.upstreams["tests"] = envStr("MODULE_TESTS_URL", "");
    c.upstreams["attempts"] = envStr("MODULE_ATTEMPTS_URL", "");
    c.upstreams["answers"] = envStr("MODULE_ANSWERS_URL", "");

    if (c.jwtSecret.empty()) {
        r.error = "JWT_SECRET is not set. Copy .env.example to .env and set a strong secret.";
        return r;
    }
    for (const auto& kv : c.upstreams) {
        if (!kv.second.empty() && kv.second.compare(0, 7, "http://") != 0) {
            r.error = "Upstream URL for '" + kv.first + "' must start with http:// (got '" +
                      kv.second + "').";
            return r;
        }
    }
    r.ok = true;
    return r;
}

}  // namespace gateway
