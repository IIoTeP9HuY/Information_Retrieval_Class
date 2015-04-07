cmake_minimum_required(VERSION 2.6)

include_directories("../../include")

set(SIMHASH_SRC_LIST "main.cpp")
file(GLOB SIMHASH_HEADERS "*.hpp")
list(APPEND SIMHASH_SRC_LIST ";" ${SIMHASH_HEADERS})

# set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_BUILD_TYPE RELEASE)

set(CMAKE_CXX_FLAGS_DEBUG "-std=c++0x -Wall -O0")

set(CMAKE_CXX_FLAGS_RELEASE "--std=c++0x -Wall -O3")

add_executable(Simhash ${SIMHASH_SRC_LIST})
add_executable(extract "extract.cpp")

find_package(Boost COMPONENTS system filesystem regex program_options REQUIRED)

find_package(LibXML++ REQUIRED)
include_directories(${LibXML++_INCLUDE_DIRS} /usr/lib/libxml++-2.6/include)

find_package(GLIBMM2 REQUIRED)
include_directories(${GLIBMM2_INCLUDE_DIR})

FIND_PACKAGE(GLIB REQUIRED)
include_directories(${GLIB_INCLUDE_DIRS})

include_directories(/usr/local/lib/include/)

find_library(TIDY_LIBRARY tidy
	PATHS /usr/local/lib)

target_link_libraries(Simhash ${Boost_LIBRARIES} filecrawler ${LibXML++_LIBRARIES} ${GLIBMM2_LIBRARY} ${TIDY_LIBRARY})

target_link_libraries(extract ${Boost_LIBRARIES} filecrawler ${LibXML++_LIBRARIES} ${GLIBMM2_LIBRARY} ${TIDY_LIBRARY})
