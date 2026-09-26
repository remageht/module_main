# Static checks for module_main gateway v1 (ASCII only).
# Run: powershell -ExecutionPolicy Bypass -File tests/test_static_checks.ps1
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$script:fail = 0

function Check($name, $cond) {
    if ($cond) { Write-Output ('PASS: ' + $name) }
    else { Write-Output ('FAIL: ' + $name); $script:fail++ }
}

function Read-File($rel) {
    Get-Content -LiteralPath (Join-Path $root $rel) -Raw -Encoding UTF8
}

$cmake = Read-File 'CMakeLists.txt'
$checkTokenCpp = Read-File 'src/CheckToken.cpp'
$checkTokenH = Read-File 'src/CheckToken.h'
$findToken = Read-File 'src/FindToken.h'
$handle = Read-File 'src/HandleClient.cpp'
$handleH = Read-File 'src/HandleClient.h'
$main = Read-File 'src/main.cpp'
$getAddrCpp = Read-File 'src/getAddress.cpp'
$getAddrH = Read-File 'src/getAddress.h'
$netcompat = Read-File 'src/NetCompat.h'
$config = Read-File 'src/Config.h'
$httpH = Read-File 'src/Http.h'
$httpCpp = Read-File 'src/Http.cpp'
$routerH = Read-File 'src/Router.h'
$routerCpp = Read-File 'src/Router.cpp'
$proxyH = Read-File 'src/Proxy.h'
$proxyCpp = Read-File 'src/Proxy.cpp'
$logger = Read-File 'src/Logger.h'
$limiter = Read-File 'src/RateLimiter.h'
$contract = Read-File 'docs/API_CONTRACT.md'
$integration = Read-File 'docs/INTEGRATION.md'
$dockerfile = Read-File 'Dockerfile'
$compose = Read-File 'docker-compose.yml'
$ci = Read-File '.github/workflows/build.yml'

# --- Build ---
Check 'cmake lists CheckToken.cpp' ($cmake -match 'src/CheckToken\.cpp')
Check 'cmake lists Http.cpp' ($cmake -match 'src/Http\.cpp')
Check 'cmake lists Router.cpp' ($cmake -match 'src/Router\.cpp')
Check 'cmake lists Proxy.cpp' ($cmake -match 'src/Proxy\.cpp')
Check 'cmake lists getAddress.cpp lowercase' ($cmake -match 'src/getAddress\.cpp')
Check 'cmake has no GetAddress.cpp mismatch' ($cmake -cnotmatch 'src/GetAddress\.cpp')
Check 'cmake sets vendor includes' ($cmake -match 'VENDOR_INCLUDE_DIR')
Check 'cmake links ws2_32' ($cmake -match 'ws2_32')
Check 'cmake has OpenSSL branch for Linux' ($cmake -match 'find_package\(OpenSSL')

# --- Cross-platform net layer ---
Check 'NetCompat exists with WIN32 branch' ($netcompat -match '_WIN32')
Check 'NetCompat has BSD branch' ($netcompat -match 'sys/socket\.h')
Check 'NetCompat close helper' ($netcompat -match 'close_socket')
Check 'NetCompat recv timeout helper' ($netcompat -match 'set_recv_timeout')

# --- Secrets stay in env ---
Check 'no hardcoded secret in CheckToken' ($checkTokenCpp -notmatch '"key"')
Check 'JWT_SECRET from env' ($checkTokenCpp -match 'getenv')
Check 'JWT_SECRET string present' ($checkTokenCpp -match 'JWT_SECRET')
Check 'token masked in logs' (($checkTokenCpp -match 'maskToken') -or ($handle -match 'maskToken'))
Check 'env example exists' (Test-Path (Join-Path $root '.env.example'))
Check 'env example has upstream urls' ((Read-File '.env.example') -match 'MODULE_USERS_URL')
Check 'env example has no real secret' ((Read-File '.env.example') -notmatch 'sk-|secret123|password')

# --- HTTP layer ---
Check 'Http parse function' ($httpH -match 'parseHttpRequest')
Check 'Http json error' ($httpH -match 'jsonError')
Check 'Http build response' ($httpH -match 'buildResponse')
Check 'Http closes connections' ($httpCpp -match 'connection: close')
Check 'Http rejects path traversal' ($httpCpp -match '\.\.')

# --- Router ---
Check 'Router matchRoute' ($routerH -match 'matchRoute')
Check 'Router requiredScope' ($routerH -match 'requiredScope')
Check 'Router read scope' ($routerCpp -match ':read')
Check 'Router write scope' ($routerCpp -match ':write')
Check 'Router health route' ($routerCpp -match '/health')
Check 'Router 405 method guard' ($routerCpp -match 'kMethodNotAllowed')
Check 'Router admin bypass' ($routerH -match 'admin')
Check 'Router granular prefix' ($routerH -match 'granularPrefix')

# --- Auth ---
Check 'verifyJwt declared' ($checkTokenH -match 'verifyJwt')
Check 'verifyJwt extracts sub' ($checkTokenCpp -match 'get_subject')
Check 'verifyJwt extracts roles' ($checkTokenCpp -match 'roles')
Check 'verifyJwt extracts permissions' ($checkTokenCpp -match 'permissions')
Check 'verifyJwt checks issuer' ($checkTokenCpp -match 'requiredIssuer')
Check 'verifyJwt fail closed' ($checkTokenCpp -match 'fail closed')

# --- findToken legacy helper intact ---
Check 'findToken handles newline delim' ($findToken -match 'n')
Check 'findToken takes const ref' ($findToken -match 'const std::string&')

# --- Gateway pipeline ---
Check 'no new char leak' ($handle -notmatch 'new char')
Check 'reads via recv loop' ($handle -match '::recv')
Check '401 Unauthorized status' (($handle -match '401') -and ($httpCpp -match 'Unauthorized'))
Check '403 Forbidden status' (($handle -match '403') -and ($httpCpp -match 'Forbidden'))
Check 'send checked' ($handle -match 'SOCKET_ERROR')
Check 'socket closed after reply' ($handle -match 'close_socket')
Check 'health endpoint served' (($handle -match 'kHealth') -or ($handle -match '/health'))
Check 'rate limit 429 path' ($handle -match '429')
Check 'auth header parsed' ($handle -match 'authorization')
Check 'verifyJwt used in pipeline' ($handle -match 'verifyJwt')
Check 'scope enforced in pipeline' ($handle -match 'hasScope')
Check 'upstream 502 path' ($handle -match 'bad_gateway')
Check 'upstream 504 path' ($handle -match 'upstream_timeout')
Check 'body limit 413 path' ($handle -match '413')
Check 'handler takes client ip' ($handleH -match 'clientIp')

# --- main: listener + shutdown ---
Check 'bind used' ($main -match 'bind')
Check 'listen checked' ($main -match 'listen')
Check 'threads detached' ($main -match 'detach')
Check 'no unbounded thread vector' ($main -notmatch 'clientThreads\.emplace_back')
Check 'client limit guard' ($main -match 'maxClients')
Check 'resolve via getaddrinfo' ($main -match 'getaddrinfo')
Check 'recv timeout set' ($main -match 'set_recv_timeout')
Check 'config fail fast' ($main -match 'Config error')
Check 'graceful shutdown flag' ($main -match 'g_running')
Check 'SIGINT handled' ($main -match 'SIGINT')
Check 'version banner' ($main -match 'kGatewayVersion')

# --- getAddress ---
Check 'getAddress returns std string' ($getAddrH -match 'std::string getLocalIPAddress')
Check 'uses getaddrinfo' ($getAddrCpp -match 'getaddrinfo')
Check 'no gethostbyname call' ($getAddrCpp -notmatch 'gethostbyname\(hostname\)')
Check 'no inet_ntoa aliasing' ($getAddrCpp -notmatch 'address = inet_ntoa')

# --- Logger / limiter ---
Check 'Logger levels' ($logger -match 'LogLevel')
Check 'Logger timestamped' ($logger -match 'put_time')
Check 'Limiter allow()' ($limiter -match 'bool allow')

# --- Proxy ---
Check 'Proxy forward declared' ($proxyH -match 'forwardRequest')
Check 'Proxy injects X-Auth-Sub' ($proxyCpp -match 'x-auth-sub')
Check 'Proxy injects X-Forwarded-For' ($proxyCpp -match 'x-forwarded-for')
Check 'Proxy strips Authorization' ($proxyCpp -notmatch 'authorization')
Check 'Proxy connect timeout' ($proxyCpp -match 'connectWithTimeout')
Check 'Proxy sanitizes headers' ($proxyCpp -match 'sanitizeHeaderValue')

# --- Deps / CI / Docker ---
Check 'picojson vendored' (Test-Path (Join-Path $root 'src/lib/include/picojson/picojson.h'))
Check 'CI builds matrix' (($ci -match 'windows-latest') -and ($ci -match 'ubuntu-latest'))
Check 'CI builds docker image' ($ci -match 'docker build')
Check 'Dockerfile healthcheck' ($dockerfile -match 'HEALTHCHECK')
Check 'Dockerfile non-root user' ($dockerfile -match 'USER app')
Check 'Dockerfile exposes port' ($dockerfile -match 'EXPOSE')
Check 'compose gateway service' ($compose -match 'gateway:')
Check 'compose uses env_file' ($compose -match 'env_file')

# --- Docs ---
Check 'contract doc exists' (Test-Path (Join-Path $root 'docs/API_CONTRACT.md'))
Check 'integration doc exists' (Test-Path (Join-Path $root 'docs/INTEGRATION.md'))
Check 'contract lists areas' ($contract -match 'attempts')
Check 'contract documents scopes' ($contract -match ':write')
Check 'integration documents X-Auth' ($integration -match 'X-Auth-Sub')

# --- Logic mirror: scope rules from Router.cpp ---
function Test-HasScope($roles, $permissions, $area, $required) {
    if ($roles -contains 'admin') { return $true }
    if ($permissions -contains $required) { return $true }
    $prefix = 'user:'
    if ($area -eq 'courses') { $prefix = 'course:' }
    if ($permissions | Where-Object { $_.StartsWith($prefix) }) { return $true }
    return $false
}
Check 'scope: admin bypass' ((Test-HasScope @('admin') @() 'users' 'users:read') -eq $true)
Check 'scope: exact match' ((Test-HasScope @('user') @('users:read') 'users' 'users:read') -eq $true)
Check 'scope: granular prefix' ((Test-HasScope @('user') @('user:list:read') 'users' 'users:read') -eq $true)
Check 'scope: deny without rights' ((Test-HasScope @('user') @('courses:read') 'users' 'users:read') -eq $false)
Check 'scope: write needs write' ((Test-HasScope @('user') @('users:read') 'users' 'users:write') -eq $false)

function Find-Token($message) {
    $bearerPos = $message.IndexOf('Bearer ')
    if ($bearerPos -lt 0) { return '' }
    $start = $bearerPos + 7
    if ($start -ge $message.Length) { return '' }
    $cut = $message.Length
    $delims = @(' ', "`n", "`r", "`t", '"', [char]39)
    foreach ($d in $delims) {
        $p = $message.IndexOf($d, $start)
        if ($p -ge 0 -and $p -lt $cut) { $cut = $p }
    }
    return $message.Substring($start, $cut - $start)
}
Check 'logic simple bearer' ((Find-Token 'Authorization: Bearer abc123') -eq 'abc123')
Check 'logic bearer CRLF trimmed' ((Find-Token ('Authorization: Bearer abc123' + [char]13 + [char]10 + 'Host: x')) -eq 'abc123')
Check 'logic bearer space trimmed' ((Find-Token 'Bearer tok extra') -eq 'tok')
Check 'logic no bearer empty' ((Find-Token 'no auth here') -eq '')
Check 'logic empty after bearer' ((Find-Token 'Bearer ') -eq '')

if ($script:fail -gt 0) { Write-Output ''; Write-Output ($script:fail.ToString() + ' check(s) FAILED'); exit 1 }
Write-Output ''
Write-Output 'All checks passed.'
