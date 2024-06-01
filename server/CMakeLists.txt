cmake_minimum_required(VERSION 3.28)

project(argon_vpn)

set(CMAKE_CXX_STANDARD 17)

LINK_LIBRARIES("ws2_32")

add_executable(argon_vpn main.cpp)
add_executable(client_test client.cpp)