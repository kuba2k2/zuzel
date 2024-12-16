cmake_minimum_required(VERSION 3.20)

file(GLOB SOURCES "*.c" "*/*.c" "*/*/*.c")
add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
target_include_directories(${PROJECT_NAME} PRIVATE .)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2main SDL2::SDL2 uthash cjson-static)
