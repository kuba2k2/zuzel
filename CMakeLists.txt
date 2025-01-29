cmake_minimum_required(VERSION 3.20)

project(zuzel C)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

if (MSVC)
    add_compile_definitions(MSVC=1)
    # disable warnings
    add_compile_options(
            /wd4820 /wd4668 /wd4255 /wd4100 /wd4996 /wd5045 /wd4061 /wd4206 /wd4295 /wd4132 /wd4701 /wd4702
            /wd4191 /wd4242 /wd4389 /wd4018 /wd4293 /wd4388 /wd4267 /wd4244 /wd4201
    )
    # remove unused code
    add_compile_options(/Gy)
else ()
    set(CMAKE_C_STANDARD 11)
    # disable warnings
    add_compile_options(-Wno-attributes -Wno-dangling-pointer)
    # remove unused code
    add_compile_options(-fdata-sections -ffunction-sections)
    add_link_options(-Wl,--gc-sections)
endif ()

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
if (UNIX OR MSVC)
    set(BUILD_SHARED_LIBS OFF)
endif ()

add_subdirectory("lib/")
add_subdirectory("res/")
add_subdirectory("src/")

function(print_all_targets DIR)
    get_property(TARGETS DIRECTORY "${DIR}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach (TARGET IN LISTS TARGETS)
        string(REPLACE "${CMAKE_CURRENT_LIST_DIR}" "" RELDIR "${DIR}")
        message(STATUS "Target: ${TARGET} | .${RELDIR}/")
    endforeach ()

    get_property(SUBDIRS DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
    foreach (SUBDIR IN LISTS SUBDIRS)
        print_all_targets("${SUBDIR}")
    endforeach ()
endfunction()

print_all_targets("${CMAKE_CURRENT_LIST_DIR}")
