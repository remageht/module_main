cmake_minimum_required(VERSION 3.10)
project(MODULE_MAIN)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

include_directories(include third_party/cpp-httplib third_party/nlohmann) #заголовочные файлы


add_executable(module_main src/main.cpp src/API.cpp src/Database.cpp) #основной исполняемый файл


add_executable(test_main tests/test_main.cpp src/Database.cpp) #юнит тесты


target_link_libraries(module_main PRIVATE Threads::Threads) #основной исполняемый файл
target_link_libraries(test_main PRIVATE Threads::Threads) #сделан отдельно от основного исполнямего файла