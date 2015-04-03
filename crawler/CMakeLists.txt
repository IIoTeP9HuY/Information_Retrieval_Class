cmake_minimum_required(VERSION 2.8)

project(crawler)

set(SRC_LIST
	main.cpp
)

add_executable(${PROJECT_NAME}
	${SRC_LIST}
)

add_definitions(-O2)
add_definitions(-std=c++0x)

find_package(Boost COMPONENTS system filesystem regex program_options REQUIRED)

set(LIBRARIES
	boost_filesystem
	boost_system
	boost_regex
	boost_program_options
	curl
	pthread
)

target_link_libraries(${PROJECT_NAME}
	${LIBRARIES}
	${Boost_LIBRARIES}
)
