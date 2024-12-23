project(cpplox CXX)
cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

add_executable(
    cpplox
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/interpreter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/environment.cpp
)

target_include_directories(cpplox PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
