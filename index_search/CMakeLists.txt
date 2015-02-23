project(irindexer)

cmake_minimum_required(VERSION 2.6)

set(SRC_LIST irindexer.cpp search_engine.hpp)

set(CMAKE_CXX_FLAGS "--std=c++0x -Wall -O2")

add_executable(${PROJECT_NAME} ${SRC_LIST})
