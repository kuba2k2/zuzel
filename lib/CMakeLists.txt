cmake_minimum_required(VERSION 3.20)

set(SDL_AUDIO OFF)
set(SDL_POWER OFF)
set(SDL_SENSOR OFF)
set(SDL_VIDEO OFF)
set(SDL_LOADSO OFF)
set(SDL_ATOMIC OFF)

set(SDL_STATIC ON CACHE INTERNAL "")
set(SDL_CMAKE_DEBUG_POSTFIX "" CACHE INTERNAL "")
add_subdirectory(sdl)

add_library(uthash INTERFACE)
target_include_directories(uthash INTERFACE "uthash/src/")
