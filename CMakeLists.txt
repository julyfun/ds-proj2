cmake_minimum_required(VERSION 3.10)
project(ds-proj2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Debug") # 这样才能启用 assert
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory("fmt")
include_directories("fmt/include")

include_directories(src)
add_executable(run src/main.cpp)
target_link_libraries(run fmt::fmt)
