cmake_minimum_required(VERSION 3.20)

project(zuzel C)

set(CMAKE_C_STANDARD 11)

add_compile_options(-fdata-sections -ffunction-sections)
add_link_options(-Wl,--gc-sections)

file(GLOB SOURCES "src/*.c" "src/*/*.c")
add_executable(${PROJECT_NAME} ${SOURCES})

set(SDL_STATIC ON CACHE INTERNAL "")
set(SDL_CMAKE_DEBUG_POSTFIX "" CACHE INTERNAL "")
add_subdirectory(sdl)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)
