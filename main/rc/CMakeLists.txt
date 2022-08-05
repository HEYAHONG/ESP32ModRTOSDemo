
find_program(FSGEN NAMES fsgen  PATHS . NO_CACHE )
if(EXISTS ${FSGEN})
    message("rc fsgen:${FSGEN}")
else()
    message("fsgen is not found,try to compile!")
    find_program(G++ NAMES g++  c++ REQUIRED)
    if(NOT EXISTS ${G++})
        message(FATAL_ERROR "g++ is not found")
    endif()
    execute_process(
                        COMMAND ${G++} -static -static-libgcc -static fsgen.cpp -o fsgen
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        )
    find_program(FSGEN NAMES fsgen fsgen.exe PATHS . REQUIRED NO_CACHE )
endif()
file(GLOB_RECURSE FS_FILES fs/*)
add_custom_command(
                     COMMAND ${FSGEN} ${CMAKE_CURRENT_SOURCE_DIR}/fs ${CMAKE_CURRENT_SOURCE_DIR}/RC_fs.c
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     OUTPUT  ${CMAKE_CURRENT_SOURCE_DIR}/RC_fs.c
                     DEPENDS ${FS_FILES}
)

add_library(RC STATIC RC.h RC.c ${CMAKE_CURRENT_SOURCE_DIR}/RC_fs.c)
target_include_directories(RC PUBLIC .)
