cmake_minimum_required(VERSION 2.6)

include_directories("../../include")

aux_source_directory(. SIMHASH_SRC_LIST)
file(GLOB SIMHASH_HEADERS "*.hpp")
list(APPEND SIMHASH_SRC_LIST ";" ${SIMHASH_HEADERS})

# set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_BUILD_TYPE RELEASE)

set(CMAKE_CXX_FLAGS_DEBUG "-std=c++0x -Wall -O0")

set(CMAKE_CXX_FLAGS_RELEASE "--std=c++0x -Wall -O3")

add_executable(Simhash ${SIMHASH_SRC_LIST})

find_package(Boost COMPONENTS system filesystem regex program_options REQUIRED)

find_package(LibXML++ REQUIRED)
include_directories(${LIBXMLPP_INCLUDE_DIR} /usr/lib/libxml++-2.6/include)

find_package(GLIBMM2 REQUIRED)
include_directories(${GLIBMM2_INCLUDE_DIR})

FIND_PACKAGE(GLIB REQUIRED)
include_directories(${GLIB_INCLUDES})

include_directories(/usr/local/lib/include/)

find_library(TIDY_LIBRARY tidy 
	PATHS /usr/local/lib)

target_link_libraries(Simhash ${Boost_LIBRARIES} filecrawler ${LIBXMLPP_LIBRARY} ${GLIBMM2_LIBRARY} ${TIDY_LIBRARY})