cmake_minimum_required(VERSION 3.16)
project(MainModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Vendor include dirs (openssl + jwt-cpp + picojson live under src/lib).
# NOTE: long-term prefer find_package(OpenSSL) + FetchContent(jwt-cpp);
# this keeps compatibility with the existing repo layout.
set(VENDOR_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/lib/include")
set(VENDOR_STATIC_LIB_DIR "${CMAKE_SOURCE_DIR}/src/lib/static_lib")

add_executable(main_module
  src/main.cpp
  src/HandleClient.cpp
  src/CheckToken.cpp
  src/Http.cpp
  src/Router.cpp
  src/Proxy.cpp
  src/getAddress.cpp
)

target_include_directories(main_module PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${VENDOR_INCLUDE_DIR}"
)

find_package(Threads REQUIRED)

if(WIN32)
  # ws2_32 needed by MinGW and MSVC; pragma comment covers MSVC only.
  target_link_libraries(main_module PRIVATE ws2_32 Threads::Threads)
  # Link vendored libcrypto only when the file really exists (Windows build).
  if(EXISTS "${VENDOR_STATIC_LIB_DIR}/libcrypto.lib")
    target_link_libraries(main_module PRIVATE "${VENDOR_STATIC_LIB_DIR}/libcrypto.lib")
  endif()
else()
  find_package(OpenSSL REQUIRED)
  target_link_libraries(main_module PRIVATE OpenSSL::Crypto Threads::Threads)
endif()

if(MSVC)
  target_compile_options(main_module PRIVATE /W4 /permissive-)
else()
  target_compile_options(main_module PRIVATE -Wall -Wextra -Wpedantic)
endif()
