cmake_minimum_required(VERSION 3.10)
project(Module_Main)

set(CMAKE_CXX_STANDARD 17)  #стандарт изменен с 11 на 17



find_package(Threads REQUIRED) #подключение сторонней библиотеки

add_executable(module_main src/main.cpp src/Database.cpp src/API.cpp) #добавление исходников

target_link_libraries(module_main PRIVATE Threads::Threads) #подключение бибилитек

