cmake_minimum_required(VERSION 3.17)
project(cyield C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(cyield
        cyield.c
        cyield.h main.c)
