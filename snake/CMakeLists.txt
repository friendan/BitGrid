#cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
#cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
#cmake --build build --config Debug
#cmake --build build --config Release
cmake_minimum_required(VERSION 3.20)
project(SnakeD2D LANGUAGES CXX RC)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ===================== 设置静态链接 CRT =====================
if(MSVC)
    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${conf} CONF_UPPER)
        if(CONF_UPPER STREQUAL "DEBUG")
            set(CMAKE_CXX_FLAGS_${CONF_UPPER} "${CMAKE_CXX_FLAGS_${CONF_UPPER}} /MTd")
        else()
            set(CMAKE_CXX_FLAGS_${CONF_UPPER} "${CMAKE_CXX_FLAGS_${CONF_UPPER}} /MT")
        endif()
        # 移除动态 CRT 的选项（防止冲突）
        string(REPLACE "/MDd" "" CMAKE_CXX_FLAGS_${CONF_UPPER} "${CMAKE_CXX_FLAGS_${CONF_UPPER}}")
        string(REPLACE "/MD" "" CMAKE_CXX_FLAGS_${CONF_UPPER} "${CMAKE_CXX_FLAGS_${CONF_UPPER}}")
    endforeach()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

remove_definitions(-D_MBCS)
add_definitions(-DUNICODE -D_UNICODE)

# 强制MSVC编译器用UTF-8解析源码
add_compile_options(/utf-8)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TARGET_NAME SnakeD2D)
file(GLOB_RECURSE SRC_FILES
    ${SRC_DIR}/*.hpp
    ${SRC_DIR}/*.cpp
    ${SRC_DIR}/*.rc
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_FILES})

add_executable(${TARGET_NAME} WIN32 ${SRC_FILES})
target_link_libraries(${TARGET_NAME} PRIVATE shell32.lib comctl32.lib shlwapi.lib winmm.lib d2d1.lib dwrite.lib gdiplus.lib)

set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME_RELEASE "GridGame"
    OUTPUT_NAME_DEBUG   "GridGame"
)









































