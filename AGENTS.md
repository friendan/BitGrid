# BitGrid 项目结构

## 项目概述
BitGrid 是一个基于 ezui UI 库开发的 WIN32 UI 程序，使用 CMake 作为构建系统。

## 目录结构
```
BitGrid/
├── ezui/              # ezui UI 库
│   ├── include/       # 头文件
│   └── sources/       # 源文件
├── CMakeLists.txt     # CMake 构建配置
├── AGENTS.md          # 项目组件说明
├── main.cpp           # 主入口文件
└── README.md          # 项目说明
```

## ezui 库组件

### 核心组件
- **Application** - 应用程序类，管理整个应用的生命周期
- **Window** - 窗口基类，提供窗口基本功能
- **Control** - 控件基类，所有UI控件的父类

### 布局组件
- **HLayout** - 水平布局
- **VLayout** - 垂直布局
- **TabLayout** - 标签页布局

### 基础控件
- **Button** - 按钮
- **Label** - 标签
- **TextBox** - 文本框
- **CheckBox** - 复选框
- **RadioButton** - 单选按钮
- **ComboBox** - 下拉组合框
- **PictureBox** - 图片框
- **Spacer** - 间隔控件

### 高级控件
- **HListView** - 水平列表视图
- **VListView** - 垂直列表视图
- **TileListView** - 瓷砖式列表视图
- **PagedListView** - 分页列表视图
- **HScrollBar** - 水平滚动条
- **VScrollBar** - 垂直滚动条

### 窗口相关
- **BorderlessWindow** - 无边框窗口
- **LayeredWindow** - 分层窗口
- **PopupWindow** - 弹出窗口
- **IFrame** -  iframe 窗口

### 其他组件
- **Animation** - 动画支持
- **Bitmap** - 位图处理
- **Direct2DRender** - Direct2D 渲染
- **Menu** - 菜单
- **NotifyIcon** - 系统托盘图标
- **ShadowBox** - 阴影框
- **Task** - 任务管理
- **Timer** - 定时器
- **UIManager** - UI管理器
- **UISelector** - UI选择器
- **UIString** - UI字符串处理

## 使用说明

### 基本步骤
1. 包含必要的头文件
2. 创建 Application 实例
3. 创建 Window 实例
4. 添加控件到窗口
5. 启动应用程序

### 示例代码
```cpp
#include <EzUI/EzUI.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    EzUI::Application app;
    
    auto window = EzUI::Window::Create(L"BitGrid", 800, 600);
    window->SetBackgroundColor(EzUI::Color::White);
    
    auto button = EzUI::Button::Create(L"Click Me");
    button->SetPosition(100, 100);
    button->SetSize(200, 50);
    button->OnClick([](EzUI::Control* sender) {
        EzUI::MessageBox::Show(L"Hello, BitGrid!");
    });
    
    window->AddChild(button);
    window->Show();
    
    return app.Run();
}
```

## 构建说明

### 环境要求
- Visual Studio 2020
- CMake 3.16 或更高版本
- Windows SDK

### 构建步骤
1. 创建构建目录：`mkdir build && cd build`
2. 运行 CMake：`cmake ..`
3. 打开生成的解决方案：`BitGrid.sln`
4. 编译并运行项目

## 注意事项
- 确保使用 C++17 或更高版本
- 链接必要的系统库（user32, gdi32, comctl32 等）
- 运行时需要 Direct2D 支持
