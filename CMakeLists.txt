cmake_minimum_required(VERSION 3.20.0)

add_library(tm1637 ./tm1637.c ./tm1637.h)
target_link_libraries(tm1637 PUBLIC zephyr_interface)
target_include_directories(tm1637 PUBLIC ./)
target_link_libraries(app PRIVATE tm1637)
