project(cpplox CXX)
cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
    cpplox
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_include_directories(cpplox PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
