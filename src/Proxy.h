#pragma once
// Blocking HTTP/1.1 forward proxy to upstream modules (one request per call).

#include <string>

#include "Http.h"
#include "CheckToken.h"

namespace gateway {

struct ProxyResult {
    bool ok = false;
    int status = 502;             // upstream status when ok
    std::string contentType;      // from upstream (may be empty)
    std::string body;
    std::string error;            // "connect" | "timeout" | "protocol" | ...
    bool timedOut = false;        // true -> caller should answer 504
};

// Forward req (method+headers+body) to upstreamBase + forwardPath.
// Injects X-Auth-Sub/Roles/Scopes + X-Forwarded-For, strips Authorization.
ProxyResult forwardRequest(const std::string& upstreamBase, const HttpRequest& req,
                           const std::string& forwardPath, const AuthInfo& auth,
                           int timeoutMs);

}  // namespace gateway
