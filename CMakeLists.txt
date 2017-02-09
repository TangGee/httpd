cmake_minimum_required(VERSION 3.6)
project(myhttpd)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES httpd.c httpd.h)
add_executable(myhttpd ${SOURCE_FILES})