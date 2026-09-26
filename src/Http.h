#pragma once
// Minimal HTTP/1.x request parser + response builder for the gateway.
// One request per connection (Connection: close). No chunked encoding.

#include <map>
#include <string>

namespace gateway {

struct HttpRequest {
    std::string method;   // "GET", ...
    std::string target;   // raw request-target "/api/users/5?x=1"
    std::string path;     // "/api/users/5"
    std::string query;    // "x=1" (may be empty)
    std::string version;  // "HTTP/1.1"
    std::map<std::string, std::string> headers;  // lowercased names
    std::string body;
    std::string clientIp;

    std::string header(const std::string& name) const {
        auto it = headers.find(name);
        return it == headers.end() ? "" : it->second;
    }
};

struct HttpResponse {
    int status = 200;
    std::map<std::string, std::string> headers;
    std::string body;
};

std::string toLower(std::string s);
std::string trimStr(const std::string& s);
std::string reasonPhrase(int status);

// Parse request head (everything before the empty line). Returns false on bad format.
bool parseHttpRequest(const std::string& head, HttpRequest& out);

// JSON error response: {"error":"<code>","message":"<msg>"}.
HttpResponse jsonError(int status, const std::string& code, const std::string& message);

// Serialize to wire format with Content-Type/Length + Connection: close.
std::string buildResponse(const HttpResponse& r);

}  // namespace gateway
