cmake_minimum_required(VERSION 3.16)
project(MainModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Вендорные include-директории (openssl + jwt-cpp лежат в src/lib).
# NOTE: для Linux/macOS лучше перейти на find_package(OpenSSL) + FetchContent(jwt-cpp),
# текущий вариант сохраняет совместимость с существующей структурой репо.
set(VENDOR_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/lib/include")
set(VENDOR_STATIC_LIB_DIR "${CMAKE_SOURCE_DIR}/src/lib/static_lib")

add_executable(main_module
  src/main.cpp
  src/HandleClient.cpp
  src/CheckToken.cpp
  src/getAddress.cpp
)

target_include_directories(main_module PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${VENDOR_INCLUDE_DIR}"
)

find_package(Threads REQUIRED)

if(WIN32)
  # ws2_32 нужен MinGW и MSVC; pragma comment покрывает только MSVC.
  target_link_libraries(main_module PRIVATE ws2_32 Threads::Threads)
  # Линкуем вендорный libcrypto только если файл реально существует (Windows-сборка).
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
