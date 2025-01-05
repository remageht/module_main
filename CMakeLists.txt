cmake_minimum_required(VERSION 3.10)
project(MainModule)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)



include_directories(third_party/nlohmann/single_include) #библиотека nlohmann/json
include_directories(third_party/pqxx/include)
include_directories(third_party/hiredis)
include_directories(third_party/catch2)

add_executable(module_main src/main.cpp src/API.cpp src/Database.cpp)


target_link_libraries(module_main
    PRIVATE Threads::Threads
    PRIVATE pqxx
    PRIVATE hiredis
    
)


