cmake_minimum_required(VERSION 3.20)

# Common library
file(GLOB SOURCES "core/*.c" "game/*.c" "game/player/*.c" "game/match/*.c" "net/*.c")
add_library(${PROJECT_NAME}-common STATIC ${SOURCES})
add_dependencies(${PROJECT_NAME}-common version)

target_compile_options(${PROJECT_NAME}-common PUBLIC -Wall)
target_include_directories(${PROJECT_NAME}-common PUBLIC "." "../res/")

target_link_libraries(${PROJECT_NAME}-common PUBLIC SDL2::SDL2main SDL2::SDL2)
target_link_libraries(${PROJECT_NAME}-common PUBLIC uthash)
target_link_libraries(${PROJECT_NAME}-common PUBLIC cjson-static)
target_link_libraries(${PROJECT_NAME}-common PUBLIC ssl)

if (WIN32)
    target_link_libraries(${PROJECT_NAME}-common PUBLIC ws2_32)
    if (!MSVC)
        target_link_libraries(${PROJECT_NAME}-common PUBLIC ssp)
    endif ()
else ()
    target_link_libraries(${PROJECT_NAME}-common PUBLIC m dl)
endif ()

# UI executable
file(GLOB SOURCES "main_ui.c" "ui/*.c" "ui/*/*.c")
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}-common)

# Server executable
file(GLOB SOURCES "main_server.c")
add_executable(${PROJECT_NAME}-server ${SOURCES})
target_compile_definitions(${PROJECT_NAME}-server PRIVATE HEADLESS=1)
target_link_libraries(${PROJECT_NAME}-server PRIVATE ${PROJECT_NAME}-common)
