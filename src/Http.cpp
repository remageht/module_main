#include "Http.h"

#include <cctype>
#include <cstdio>
#include <sstream>

namespace gateway {

std::string toLower(std::string s) {
    for (auto& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return s;
}

std::string trimStr(const std::string& s) {
    size_t a = 0;
    while (a < s.size() && (s[a] == ' ' || s[a] == '\t')) ++a;
    size_t b = s.size();
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) --b;
    return s.substr(a, b - a);
}

std::string reasonPhrase(int status) {
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 429: return "Too Many Requests";
        case 502: return "Bad Gateway";
        case 504: return "Gateway Timeout";
        default: return "Error";
    }
}

static std::string jsonEscape(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    o += buf;
                } else {
                    o += c;
                }
        }
    }
    return o;
}

HttpResponse jsonError(int status, const std::string& code, const std::string& message) {
    HttpResponse r;
    r.status = status;
    r.headers["content-type"] = "application/json";
    r.body = "{\"error\":\"" + jsonEscape(code) + "\",\"message\":\"" + jsonEscape(message) + "\"}";
    return r;
}

bool parseHttpRequest(const std::string& head, HttpRequest& out) {
    out = HttpRequest();
    size_t lineEnd = head.find("\r\n");
    if (lineEnd == std::string::npos) return false;

    // Request-Line: METHOD SP TARGET SP VERSION
    {
        std::istringstream rl(head.substr(0, lineEnd));
        if (!(rl >> out.method >> out.target >> out.version)) return false;
        std::string extra;
        if (rl >> extra) return false;  // garbage after version
    }
    for (char c : out.method) {
        if (!std::isupper(static_cast<unsigned char>(c)) && c != '-') return false;
    }
    if (out.target.empty() || out.target[0] != '/') return false;
    if (out.version.compare(0, 5, "HTTP/") != 0) return false;

    size_t q = out.target.find('?');
    if (q == std::string::npos) {
        out.path = out.target;
    } else {
        out.path = out.target.substr(0, q);
        out.query = out.target.substr(q + 1);
    }
    if (out.path.find("..") != std::string::npos) return false;  // no path traversal

    // Headers
    size_t pos = lineEnd + 2;
    while (pos < head.size()) {
        size_t eol = head.find("\r\n", pos);
        if (eol == std::string::npos) return false;
        if (eol == pos) break;  // empty line = end of head
        std::string line = head.substr(pos, eol - pos);
        size_t colon = line.find(':');
        if (colon == std::string::npos) return false;
        std::string name = toLower(trimStr(line.substr(0, colon)));
        std::string value = trimStr(line.substr(colon + 1));
        if (name.empty() || value.size() > 8192) return false;
        if (out.headers.size() > 100) return false;
        out.headers[name] = value;
        pos = eol + 2;
    }
    return true;
}

std::string buildResponse(const HttpResponse& r) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << r.status << " " << reasonPhrase(r.status) << "\r\n";
    bool hasType = false, hasLen = false;
    for (const auto& kv : r.headers) {
        if (kv.first == "content-type") hasType = true;
        if (kv.first == "content-length") hasLen = true;
    }
    for (const auto& kv : r.headers) {
        oss << kv.first << ": " << kv.second << "\r\n";
    }
    if (!hasType) oss << "content-type: application/json\r\n";
    if (!hasLen) oss << "content-length: " << r.body.size() << "\r\n";
    oss << "connection: close\r\n\r\n";
    std::string head = oss.str();
    head += r.body;
    return head;
}

}  // namespace gateway
