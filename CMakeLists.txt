cmake_minimum_required(VERSION 3.10)
project(Module_Main)

set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(Module_Main src/main.cpp src/router.cpp )


