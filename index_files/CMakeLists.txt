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

add_library(filecrawler ${SRC_LIST})

find_package(Boost COMPONENTS system filesystem regex program_options REQUIRED)

target_link_libraries(filecrawler ${Boost_LIBRARIES})

add_subdirectory(examples)
