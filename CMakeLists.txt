# Dummy file just to make IDEs happy

cmake_minimum_required(VERSION 3.20)
project(my_zig_project C)

# Glob your C sources so CLion indexes them
file(GLOB_RECURSE C_SOURCES "src/*.c" "src/*.h")
add_library(dummy STATIC ${C_SOURCES})

# Add any include paths your C code needs
target_include_directories(dummy PRIVATE src)