cmake_minimum_required(VERSION 2.6)

include_directories("../../include")
include_directories("../../../")

set(URL_GRAPH_SRC_LIST "main.cpp")
file(GLOB URL_GRAPH_HEADERS "*.hpp")
list(APPEND URL_GRAPH_SRC_LIST ";" ${URL_GRAPH_HEADERS})

# set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_BUILD_TYPE RELEASE)

set(CMAKE_CXX_FLAGS_DEBUG "-std=c++0x -Wall -O0")

set(CMAKE_CXX_FLAGS_RELEASE "--std=c++0x -Wall -O3")

add_executable(Webgraph ${URL_GRAPH_SRC_LIST})
add_executable(flat_webgraph "flat_webgraph_builder.cpp")

find_package(Boost COMPONENTS system filesystem regex program_options REQUIRED)

target_link_libraries(Webgraph ${Boost_LIBRARIES} filecrawler)
target_link_libraries(flat_webgraph ${Boost_LIBRARIES} filecrawler)
