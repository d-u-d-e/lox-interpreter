cmake_minimum_required(VERSION 3.8)
project(cpplox C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

add_executable(
    clox
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/chunk.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/memory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/debug.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/value.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/compiler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scanner.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/object.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/table.c
)

target_include_directories(clox PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
