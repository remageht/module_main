# Static review checks for module_main (ASCII only).
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
$checkToken = Read-File 'src/CheckToken.cpp'
$findToken = Read-File 'src/FindToken.h'
$handle = Read-File 'src/HandleClient.cpp'
$main = Read-File 'src/main.cpp'
$getAddrCpp = Read-File 'src/getAddress.cpp'
$getAddrH = Read-File 'src/getAddress.h'

Check 'cmake lists CheckToken.cpp' ($cmake -match 'src/CheckToken\.cpp')
Check 'cmake lists getAddress.cpp lowercase' ($cmake -match 'src/getAddress\.cpp')
Check 'cmake has no GetAddress.cpp mismatch' ($cmake -cnotmatch 'src/GetAddress\.cpp')
Check 'cmake sets vendor includes' ($cmake -match 'VENDOR_INCLUDE_DIR')
Check 'cmake links ws2_32' ($cmake -match 'ws2_32')

Check 'no hardcoded secret in CheckToken' ($checkToken -notmatch '"key"')
Check 'JWT_SECRET from env' ($checkToken -match 'getenv')
Check 'JWT_SECRET string present' ($checkToken -match 'JWT_SECRET')
Check 'token masked in logs' ($checkToken -match 'maskToken')
Check 'env example exists' (Test-Path (Join-Path $root '.env.example'))

Check 'findToken handles newline delim' ($findToken -match 'n')
Check 'findToken takes const ref' ($findToken -match 'const std::string&')

Check 'no new char leak' ($handle -notmatch 'new char')
Check 'uses bytesReceived' ($handle -match 'bytesReceived')
Check '401 Unauthorized status' ($handle -match '401 Unauthorized')
Check '403 Forbidden status' ($handle -match '403 Forbidden')
Check 'send checked' ($handle -match 'SOCKET_ERROR')
Check 'closesocket present' ($handle -match 'closesocket')

Check 'bind checked' ($main -match 'bind')
Check 'listen checked' ($main -match 'listen')
Check 'threads detached' ($main -match 'detach')
Check 'no unbounded thread vector' ($main -notmatch 'clientThreads\.emplace_back')
Check 'client limit guard' ($main -match 'MAX_CLIENT_THREADS')
Check 'inet_pton used' ($main -match 'inet_pton')
Check 'recv timeout set' ($main -match 'SO_RCVTIMEO')

Check 'getAddress returns std string' ($getAddrH -match 'std::string getLocalIPAddress')
Check 'uses getaddrinfo' ($getAddrCpp -match 'getaddrinfo')
Check 'no gethostbyname' ($getAddrCpp -notmatch 'gethostbyname')
Check 'no inet_ntoa aliasing' ($getAddrCpp -notmatch 'address = inet_ntoa')

Check 'picojson vendored' (Test-Path (Join-Path $root 'src/lib/include/picojson/picojson.h'))
Check 'CI workflow exists' (Test-Path (Join-Path $root '.github/workflows/build.yml'))

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
