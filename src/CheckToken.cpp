#include "CheckToken.h"

namespace {
std::string loadJwtSecret() {
    const char* fromEnv = std::getenv("JWT_SECRET");
    if (fromEnv != nullptr && fromEnv[0] != '\0') {
        return std::string(fromEnv);
    }
    return "";
}

void extractStringList(const jwt::decoded_jwt<jwt::traits::kazuho_picojson>& decoded,
                       const std::string& claimName,
                       std::vector<std::string>& out) {
    try {
        if (!decoded.has_payload_claim(claimName)) return;
        auto claim = decoded.get_payload_claim(claimName);
        try {
            picojson::array arr = claim.as_array();
            for (const auto& item : arr) {
                if (item.is<std::string>()) out.push_back(item.get<std::string>());
            }
            return;
        } catch (...) {
            // not an array -> fall through to single-string form
        }
        try {
            out.push_back(claim.as_string());
        } catch (...) {
        }
    } catch (...) {
    }
}
}  // namespace

AuthInfo verifyJwt(const std::string& token,
                   const std::string& secret,
                   const std::string& requiredIssuer) {
    AuthInfo info;
    if (token.empty()) {
        info.error = "empty token";
        return info;
    }
    if (secret.empty()) {
        info.error = "JWT_SECRET is not set, fail closed.";
        return info;
    }
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256{secret});
        // NOTE: iss enforced manually so the claim stays optional unless configured.
        verifier.verify(decoded);

        // exp is checked inside verify(), double-check explicitly for clarity.
        if (decoded.has_expires_at()) {
            if (decoded.get_expires_at() < std::chrono::system_clock::now()) {
                info.error = "token expired";
                return info;
            }
        }

        if (!requiredIssuer.empty()) {
            std::string iss;
            try {
                iss = decoded.get_issuer();
            } catch (...) {
            }
            if (iss != requiredIssuer) {
                info.error = "issuer mismatch";
                return info;
            }
        }

        try {
            info.sub = decoded.get_subject();
        } catch (...) {
        }
        extractStringList(decoded, "roles", info.roles);
        extractStringList(decoded, "permissions", info.permissions);

        info.ok = true;
        return info;
    } catch (const jwt::error::token_verification_exception& e) {
        info.error = std::string("verification failed: ") + e.what();
        return info;
    } catch (const std::exception& e) {
        info.error = std::string("decode error: ") + e.what();
        return info;
    }
}

// Legacy wrapper (raw-TCP mode): kept for compatibility, maps verify result
// onto the old Action vocabulary. The gateway path uses verifyJwt directly.
Action CheckToken(SOCKET clientSocket, char* message, int msgSize) {
    (void)msgSize;

    beautyPrint(clientSocket, "Find token...");
    std::string raw(message != nullptr ? message : "");
    std::string token = findToken(raw);

    if (token.empty()) {
        beautyPrint(clientSocket, "Token not found.");
        return Action::Error401;
    }

    // В логах — только маска, никогда полный токен (secrets/PII).
    beautyPrint(clientSocket, "Token: " + maskToken(token));

    const std::string secret = loadJwtSecret();
    const char* issuerEnv = std::getenv("JWT_ISSUER");
    const std::string issuer = (issuerEnv != nullptr) ? issuerEnv : "";

    AuthInfo auth = verifyJwt(token, secret, issuer);
    if (!auth.ok) {
        beautyPrint(clientSocket, "Auth failed: " + auth.error);
        return Action::Error401;
    }
    return Action::Error403;
}
