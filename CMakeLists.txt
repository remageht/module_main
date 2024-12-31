cmake_minimum_required(VERSION 3.10)
project(MODULE_MAIN)

set(CMAKE_CXX_STANDARD 17)  #стандарт изменен с 11 на 17



find_package(Catch2 REQUIRED) #поиск Catch2(фреймворк для юнит тестирования)

add_executable(module_main tests/test_main.cpp) #добавление тестов

target_link_libraries(module_main PRIVATE  Catch2::Catch2) #подключение Catch2

