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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin)

remove_definitions(-D_MBCS)
add_definitions(-DUNICODE -D_UNICODE)

# 强制MSVC编译器用UTF-8解析源码
add_compile_options(/utf-8)

set(TARGET_NAME SnakeD2D)

# ===================== 引入上级目录的公共文件 =====================
# 定义上级目录路径（使用 CMAKE_CURRENT_SOURCE_DIR 指向 snake 目录）
set(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

# 公共文件列表
set(COMMON_FILES
    ${PARENT_DIR}/AppConst.hpp
    ${PARENT_DIR}/AppUtil.hpp
    ${PARENT_DIR}/AppUtil.cpp
    ${PARENT_DIR}/DrawGrid.hpp
    ${PARENT_DIR}/DrawGrid.cpp
)

# 收集 snake 自身的源文件
file(GLOB_RECURSE SNAKE_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.rc
)

# 合并所有源文件
set(ALL_SRC_FILES ${SNAKE_SRC_FILES} ${COMMON_FILES})

add_executable(${TARGET_NAME} WIN32 ${ALL_SRC_FILES})

# 添加公共文件头文件目录（上级目录）
target_include_directories(${TARGET_NAME} PRIVATE ${PARENT_DIR})
target_link_libraries(${TARGET_NAME} PRIVATE shell32.lib comctl32.lib shlwapi.lib winmm.lib d2d1.lib dwrite.lib gdiplus.lib)

set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME_RELEASE "GridGame"
    OUTPUT_NAME_DEBUG   "GridGame"
)









































