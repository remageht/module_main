#pragma once
// Per-connection gateway pipeline:
// read HTTP request -> rate limit -> route -> auth -> proxy -> respond.
// One request per connection (Connection: close).

#include <string>

#include "NetCompat.h"
#include "Config.h"
#include "RateLimiter.h"

void handleClient(SOCKET clientSocket, const std::string& clientIp,
                  const gateway::Config& cfg, gateway::RateLimiter& limiter,
                  long long connId);
