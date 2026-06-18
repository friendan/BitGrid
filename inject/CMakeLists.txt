cmake_minimum_required(VERSION 3.16)
project(SafeHelper)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DINJECT_EXPORTS)
add_definitions(-DUNICODE -D_UNICODE)
add_compile_options("/utf-8")

# 输出到 bin 目录
foreach(conf DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${conf} ${CMAKE_CURRENT_SOURCE_DIR}/../bin)
endforeach()

set(INJECT_SOURCES
    inject.cpp
)

add_library(${PROJECT_NAME} SHARED ${INJECT_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME_DEBUG "360safe_dbg"
    OUTPUT_NAME_RELEASE "360safe"
)
