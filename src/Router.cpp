#include "Router.h"

namespace gateway {

static bool isReadMethod(const std::string& m) { return m == "GET" || m == "HEAD"; }

static bool isAllowedMethod(const std::string& m) {
    return m == "GET" || m == "POST" || m == "PUT" || m == "PATCH" || m == "DELETE";
}

RouteResult matchRoute(const std::string& method, const std::string& target, const Config& cfg) {
    RouteResult r;
    // Split target into path + query (target already validated to start with '/').
    std::string path = target;
    std::string query;
    size_t q = target.find('?');
    if (q != std::string::npos) {
        path = target.substr(0, q);
        query = target.substr(q);  // keeps leading '?'
    }

    if (path == "/health" || path == "/health/") {
        r.kind = RouteResult::kHealth;
        return r;
    }

    const std::string prefix = "/api/";
    if (path.compare(0, prefix.size(), prefix) != 0) {
        r.kind = RouteResult::kNotFound;
        return r;
    }
    std::string rest = path.substr(prefix.size());  // "<area>/..."
    size_t slash = rest.find('/');
    std::string area = (slash == std::string::npos) ? rest : rest.substr(0, slash);
    if (!isApiArea(area)) {
        r.kind = RouteResult::kNotFound;
        return r;
    }
    if (!isAllowedMethod(method)) {
        r.kind = RouteResult::kMethodNotAllowed;
        return r;
    }

    std::string remainder = (slash == std::string::npos) ? "/" : rest.substr(slash);
    if (remainder.empty()) remainder = "/";
    r.kind = RouteResult::kApi;
    r.area = area;
    r.upstreamBase = cfg.upstreamFor(area);
    r.forwardPath = remainder + query;
    r.requiredScope = area + (isReadMethod(method) ? ":read" : ":write");
    return r;
}

bool hasScope(const AuthInfo& auth, const std::string& area, const std::string& requiredScope) {
    for (const auto& role : auth.roles) {
        if (role == "admin") return true;
    }
    for (const auto& p : auth.permissions) {
        if (p == requiredScope) return true;
    }
    // Legacy granular vocabulary: any "<singular>:..." permission grants area access.
    const std::string gprefix = granularPrefix(area);
    for (const auto& p : auth.permissions) {
        if (p.compare(0, gprefix.size(), gprefix) == 0) return true;
    }
    return false;
}

}  // namespace gateway
