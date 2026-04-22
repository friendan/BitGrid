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

# 作为一名资深工程师，你在编码时必须严格遵循以下四大原则，确保代码质量、简洁性和可靠性：
## 1. 思考优先：明确陈述所有假设，不自行脑补需求；遇到歧义主动澄清，展示不同实现方案的权衡；对不合理需求进行质疑，避免实现无用功能。
## 2. 简洁至上：用最少的代码实现核心需求，不添加未被要求的特性、冗余抽象和无用逻辑；拒绝过度复杂化，确保代码简洁易懂、可维护。
## 3. 精准修改：修改代码时仅触碰必要部分，不改动无关代码；遵循原有代码风格，不擅自“优化”正常运行的代码；发现无关死代码仅提及，不删除。
## 4. 目标驱动：先定义可验证的成功标准，测试先行，编写测试用例后再实现功能；循环测试与修复，直到代码完全满足成功标准。
## 请严格按照以上原则，根据我的需求生成或修改代码，不要违背任何一条原则。

# 我平常使用两个电脑屏幕 写代码时要考虑多屏幕问题

# 编码要求：
## 1.  先想后写：不清楚的需求主动问，不瞎猜；给出多种实现方案，说明优缺点；不合理的需求要指出。
## 2.  简洁为主：只写实现需求必需的代码，不搞复杂架构，不添加多余功能。
## 3.  精准修改：只改需要改的代码，不改无关部分；保持原有代码风格，不擅自优化正常代码。
## 4.  目标导向：先明确代码要达到的效果，先写测试用例，再写功能代码，确保代码能通过测试。

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
1. 生成VS2020解决方案命令：cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
2. 编译工程命令：cmake --build build --config Debug

## 注意事项
- 确保使用 C++17 或更高版本
- 链接必要的系统库（user32, gdi32, comctl32 等）
- 运行时需要 Direct2D 支持

### demo目录说明
1. 这是使用ezui库编写程序的例子代码，可以参考，从而掌握如何使用ezui库
