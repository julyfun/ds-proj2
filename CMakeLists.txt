cmake_minimum_required(VERSION 3.10)
project(ds-proj2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


include_directories(src)
add_executable(run src/main.cpp)
# target_link_libraries(run lib)
