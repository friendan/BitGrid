cmake_minimum_required(VERSION 3.16)

project(BitGrid)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 设置UTF8编码
add_compile_options("/utf-8")

# 静态链接CRT
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 设置UNICODE字符集
add_definitions(-DUNICODE -D_UNICODE)

# 包含ezui头文件目录
include_directories(ezui/include/EzUI)

# 收集ezui源文件
file(GLOB EZUI_SOURCES "ezui/sources/*.cpp")

# 添加资源文件
set(RESOURCE_FILES BitGrid.rc)

# 添加项目源文件
set(PROJECT_SOURCES
    main.cpp
    AppUtil.cpp
    DrawGrid.cpp
    PathUtil.cpp
    ScreenCapture.cpp
    ScreenSelectOverlay.cpp
)

# 添加项目头文件（让IDE能正确显示）
set(PROJECT_HEADERS
    AppUtil.hpp
    DrawGrid.hpp
    AppConst.hpp
    PathUtil.hpp
    ScreenCapture.hpp
    ScreenSelectOverlay.hpp
)

# 添加main.cpp作为主入口文件
add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${EZUI_SOURCES} ${RESOURCE_FILES})

# 链接必要的系统库
target_link_libraries(${PROJECT_NAME} PRIVATE
    user32
    gdi32
    comctl32
    ole32
    oleaut32
    uuid
    d2d1
    dwrite
    windowscodecs
)

# 忽略特定链接器警告
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "/ignore:4286 /ignore:4217"
)

# 确保资源文件依赖关系
set_property(SOURCE BitGrid.rc PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/main.htm)
