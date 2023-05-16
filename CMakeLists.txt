cmake_minimum_required(VERSION 3.25)
project(log_daemon C)

set(CMAKE_C_STANDARD 99)
add_compile_options(-Wall -Wextra)

add_executable(log_daemon main.c logger.h logger.c)