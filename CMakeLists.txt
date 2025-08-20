cmake_minimum_required(VERSION 3.10)

# Project name
project(Game C)

# Find the required packages
find_package(Curses REQUIRED)

# Add the executable
add_executable(game main.c)

# Link the curses library to the executable
target_link_libraries(game PRIVATE ${CURSES_LIBRARIES})

# Include the curses headers
target_include_directories(game PRIVATE ${CURSES_INCLUDE_DIRS})
