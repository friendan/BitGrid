cmake_minimum_required(VERSION 3.16)

project(BitGrid)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# exe 输出到 bin 目录（不按配置名分目录），.lib/.exp/.pdb 等留在 build 目录
foreach(conf DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${conf} ${CMAKE_SOURCE_DIR}/bin)
endforeach()

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

# Debug版输出加_dbg后缀
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME_DEBUG "${PROJECT_NAME}_dbg"
)

# MNN 静态库
add_subdirectory(MNN)

# MNN 头文件路径
include_directories(MNN/include)

# 链接 MNN 和系统库
# 注意：MNN 放在前面，系统库放在后面（链接顺序重要）
target_link_libraries(${PROJECT_NAME} PRIVATE
    MNN
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

############################### snake #################################

# 将 snake 作为子工程引入，独立编译为 GridGame.exe
add_subdirectory(snake)
