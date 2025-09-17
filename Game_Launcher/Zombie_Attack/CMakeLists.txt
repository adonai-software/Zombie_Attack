cmake_minimum_required(VERSION 3.10)

# Project name
project(Game C)

# Add option for debug
option(ENABLE_DEBUG "enable debugging flags" ON)

# Set compile flag
if(ENABLE_DEBUG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall")
endif()

# Find the required packages
find_package(Curses REQUIRED)
find_package(Threads REQUIRED)

# Add the executable
add_executable(game main.c)

# Link the curses library to the executable
target_link_libraries(game PRIVATE ${CURSES_LIBRARIES} ${THREADS_LIBRARIES} cjson)

# Include the curses headers
target_include_directories(game PRIVATE ${CURSES_INCLUDE_DIRS} ${THREADS_LIBRARIES})

