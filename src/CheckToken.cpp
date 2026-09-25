#include "CheckToken.h"

namespace {
std::string loadJwtSecret() {
    const char* fromEnv = std::getenv("JWT_SECRET");
    if (fromEnv != nullptr && fromEnv[0] != '\0') {
        return std::string(fromEnv);
    }
    return "";
}
} // namespace

Action CheckToken(SOCKET clientSocket, char* message, int msgSize)
{
    (void)msgSize;

    beautyPrint(clientSocket, "Find token...");
    std::string raw(message != nullptr ? message : "");
    std::string token = findToken(raw);

    if (token.empty())
    {
        beautyPrint(clientSocket, "Token not found.");
        return Action::Error401;
    }

    // В логах — только маска, никогда полный токен (secrets/PII).
    beautyPrint(clientSocket, "Token: " + maskToken(token));

    const std::string secret = loadJwtSecret();
    if (secret.empty()) {
        beautyPrint(clientSocket, "JWT_SECRET is not set, fail closed.");
        return Action::Error401;
    }

    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{ secret });
        // NOTE: with_issuer("auth-module") не требуем по умолчанию, чтобы не
        // сломать существующие токены без iss; добавить, когда issuer зафиксирован.
        verifier.verify(decoded);

        // exp проверяется внутри verify(), но дублируем явную проверку для ясности.
        if (decoded.has_expires_at()) {
            auto exp = decoded.get_expires_at();
            if (exp < std::chrono::system_clock::now()) {
                beautyPrint(clientSocket, "Token expired.");
                return Action::Error401;
            }
        }

        // TODO: picks Action by Permission/claims (user:list:read, ...).
        // Сейчас структура Permission объявлена, но маппинг scope->endpoint
        // ещё не внедрён, поэтому считаем валидный токен недостаточным
        // для ресурса по умолчанию и возвращаем 403, а не открываем доступ.
        return Action::Error403;
    } catch (const jwt::error::token_verification_exception& e) {
        beautyPrint(clientSocket, std::string("Token verification failed: ") + e.what());
        return Action::Error401;
    } catch (const std::exception& e) {
        beautyPrint(clientSocket, std::string("Token decode error: ") + e.what());
        return Action::Error401;
    }
}
