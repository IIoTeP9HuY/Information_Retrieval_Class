project(IndexFiles)

cmake_minimum_required(VERSION 2.6)

aux_source_directory("src/" SRC_LIST)
file(GLOB HEADERS "include/*.hpp")
list(APPEND SRC_LIST ";" ${HEADERS})

include_directories("include/")

# set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_BUILD_TYPE RELEASE)

set(CMAKE_CXX_FLAGS_DEBUG "-std=c++0x -Wall -O0")

set(CMAKE_CXX_FLAGS_RELEASE "--std=c++0x -Wall -O3")

add_library(index_files ${SRC_LIST})

add_executable(${PROJECT_NAME} main.cpp)

find_package(Boost COMPONENTS system filesystem regex program_options REQUIRED)

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} index_files)
