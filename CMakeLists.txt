cmake_minimum_required(VERSION 3.16)

project(BitGrid)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# 添加main.cpp作为主入口文件
add_executable(${PROJECT_NAME} WIN32 main.cpp ${EZUI_SOURCES})

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

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
