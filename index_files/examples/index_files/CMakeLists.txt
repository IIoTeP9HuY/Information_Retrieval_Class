cmake_minimum_required(VERSION 2.6)

include_directories("../../include")

aux_source_directory(. INDEX_FILES_SRC_LIST)
file(GLOB INDEX_FILES_HEADERS "*.hpp")
list(APPEND INDEX_FILES_SRC_LIST ";" ${INDEX_FILES_HEADERS})

# set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_BUILD_TYPE RELEASE)

set(CMAKE_CXX_FLAGS_DEBUG "-std=c++0x -Wall -O0")

set(CMAKE_CXX_FLAGS_RELEASE "--std=c++0x -Wall -O3")

add_executable(IndexFiles ${INDEX_FILES_SRC_LIST})

find_package(Boost COMPONENTS system filesystem regex program_options REQUIRED)

target_link_libraries(IndexFiles ${Boost_LIBRARIES} index_files)
