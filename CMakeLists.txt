cmake_minimum_required(VERSION 3.10)
project(MainModule)

set(CMAKE_CXX_STANDARD 17)


# пути для поиска заголовочных файлов и бинарных файлов
set(OpenSSL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/lib/include/openssl")
set(OpenSSL_LIBRARIES "${CMAKE_SOURCE_DIR}/src/lib/lib")
#путь для jwt-cpp
set(jwt-cpp_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/lib/include/jwt-cpp")

add_executable(main_module 
  src/main.cpp
  src/handleClient.cpp
)

target_link_libraries(main_module PRIVATE
  ${OpenSSL_LIBRARIES}/libcrypto.lib
  ${jwt-cpp_LIBRARIES}
)


