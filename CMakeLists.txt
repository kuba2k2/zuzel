cmake_minimum_required(VERSION 3.20)

project(zuzel C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

add_compile_options(-fdata-sections -ffunction-sections)
add_link_options(-Wl,--gc-sections)

add_subdirectory("lib/")
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
