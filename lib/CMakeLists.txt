cmake_minimum_required(VERSION 3.20)

include(FetchContent)

if (MSVC AND NOT BUILD_SHARED_LIBS)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(USE_STATIC_MSVC_RUNTIMES ON)
    set(SDL_FORCE_STATIC_VCRT ON)
    set(SDL_STATIC ON)
    set(SDL_SHARED OFF)
endif ()

set(ENABLE_CJSON_UNINSTALL OFF)
set(ENABLE_CJSON_TEST OFF)
set(ENABLE_TARGET_EXPORT OFF)
set(BUILD_SHARED_AND_STATIC_LIBS ON)
set(ENABLE_PUBLIC_SYMBOLS OFF)
set(ENABLE_HIDDEN_SYMBOLS ON)
add_subdirectory(cjson)
target_include_directories(cjson-static INTERFACE "cjson/")
if (NOT MSVC)
    target_compile_options(cjson-static PRIVATE -Wno-float-conversion)
endif ()

set(SDL_AUDIO OFF)
set(SDL_POWER OFF)
set(SDL_SENSOR OFF)
set(SDL_VIDEO ON)
set(SDL_ATOMIC OFF)
set(SDL_CMAKE_DEBUG_POSTFIX "")
set(SDL2_DISABLE_INSTALL ON)
set(SDL2_DISABLE_UNINSTALL ON)
add_subdirectory(sdl)

add_library(uthash INTERFACE)
target_include_directories(uthash INTERFACE "uthash/src/")

set(LIBRESSL_SKIP_INSTALL ON)
set(LIBRESSL_APPS OFF)
set(LIBRESSL_TESTS OFF)
FetchContent_Declare(
        libressl
        DOWNLOAD_EXTRACT_TIMESTAMP false
        URL https://github.com/libressl/portable/releases/download/v3.9.2/libressl-3.9.2.tar.gz
)
FetchContent_MakeAvailable(libressl)
