cmake_minimum_required(VERSION 3.10)
project(MainModule)

set(CMAKE_CXX_STANDARD 17)


# путь для openssl
set(OpenSSL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/lib/include/openssl")
# путь для библиотеки
set(OpenSSL_LIBRARIES "${CMAKE_SOURCE_DIR}/src/lib/static_lib")
#путь для jwt-cpp
set(jwt-cpp_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/lib/include/jwt-cpp")

add_executable(main_module 
  src/main.cpp
  src/HandleClient.cpp
  src/GetAddress.cpp
)

target_link_libraries(main_module PRIVATE
  ${OpenSSL_LIBRARIES}/libcrypto.lib
  ${jwt-cpp_LIBRARIES}
)


