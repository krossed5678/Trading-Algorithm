cmake_minimum_required(VERSION 3.10)
project(cpp_trading_bot)

set(CMAKE_CXX_STANDARD 17)
include_directories(include)
file(GLOB SOURCES "src/*.cpp")
add_executable(trading_bot ${SOURCES})
