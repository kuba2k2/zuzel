cmake_minimum_required(VERSION 3.25)

project(zuzel C)

set(CMAKE_C_STANDARD 11)

file(GLOB SOURCES "src/*.c" "src/*/*.c")
add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE sdl2)
