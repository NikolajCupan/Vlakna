cmake_minimum_required(VERSION 3.18)
project(Parnost C)

set(CMAKE_C_STANDARD 99)

add_executable(Parnost main.c)

target_link_libraries(Parnost pthread m)