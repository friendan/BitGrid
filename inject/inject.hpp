#pragma once

#ifdef INJECT_EXPORTS
#define INJECT_API __declspec(dllexport)
#else
#define INJECT_API __declspec(dllimport)
#endif

// 通过窗口消息通信，WM_USER + 自定义消息
// wParam = 虚拟键码（如 VK_SPACE），lParam = 0
#define WM_INJECT_PAGE_TURN (WM_USER + 300)

extern "C" {
    // 初始化 DLL，创建隐藏窗口，返回是否成功
    INJECT_API bool Inject_Init();

    // 卸载 DLL，销毁隐藏窗口
    INJECT_API void Inject_Uninit();

    // 获取隐藏窗口句柄（用于发送消息）
    INJECT_API HWND Inject_GetWindowHandle();
}
