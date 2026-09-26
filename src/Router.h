#pragma once
// Route table: /api/<area>/... -> upstream module + required scope.
// See docs/API_CONTRACT.md section 3-4.

#include <string>
#include <vector>

#include "Config.h"
#include "CheckToken.h"

namespace gateway {

inline const std::vector<std::string>& apiAreas() {
    static const std::vector<std::string> areas = {
        "users", "courses", "quests", "tests", "attempts", "answers"};
    return areas;
}

inline bool isApiArea(const std::string& area) {
    for (const auto& a : apiAreas())
        if (a == area) return true;
    return false;
}

// Singular prefix of the legacy granular permission vocabulary
// ("user:list:read" belongs to area "users", etc.).
inline std::string granularPrefix(const std::string& area) {
    if (area == "courses") return "course:";
    if (area == "quests") return "quest:";
    if (area == "tests") return "test:";
    if (area == "attempts") return "attempt:";
    if (area == "answers") return "answer:";
    return "user:";  // area == "users"
}

struct RouteResult {
    enum Kind { kHealth, kApi, kNotFound, kMethodNotAllowed } kind = kNotFound;
    std::string area;           // e.g. "users" (kApi only)
    std::string upstreamBase;   // e.g. "http://users:8081" (kApi only)
    std::string forwardPath;    // path+query to send upstream, starts with '/'
    std::string requiredScope;  // e.g. "users:read" (kApi only)
};

// method: GET/POST/PUT/PATCH/DELETE allowed for /api; others -> kMethodNotAllowed.
RouteResult matchRoute(const std::string& method, const std::string& target,
                       const Config& cfg);

// Scope check: admin role bypasses; exact "<area>:read|write" match;
// legacy granular "<singular>:..." prefix also grants the area scope.
bool hasScope(const AuthInfo& auth, const std::string& area, const std::string& requiredScope);

}  // namespace gateway
