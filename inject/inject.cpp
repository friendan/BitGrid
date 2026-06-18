#include <Windows.h>
#include <cstdio>
#include <vector>
#include "inject.hpp"

// DllMain 中创建的隐藏窗口句柄
static HWND g_hWnd = nullptr;
static HINSTANCE g_hInst = nullptr;

#pragma region 窗口枚举辅助

/// 递归获取所有子窗口（深度优先，广度结合）
static void EnumAllChildWindows(HWND hParent, std::vector<HWND>& outList)
{
    outList.push_back(hParent);
    HWND hChild = GetWindow(hParent, GW_CHILD);
    while (hChild) {
        EnumAllChildWindows(hChild, outList);
        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }
}

/// 获取 ToDesk 进程中所有可见+启用的窗口
static std::vector<HWND> GetToDeskAllWindows()
{
    std::vector<HWND> result;

    // 获取当前进程 ID
    DWORD curPid = GetCurrentProcessId();

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* pResult = reinterpret_cast<std::vector<HWND>*>(lParam);
        DWORD wndPid = 0;
        GetWindowThreadProcessId(hwnd, &wndPid);
        if (wndPid == GetCurrentProcessId()) {
            // 添加窗口及其所有子窗口
            EnumAllChildWindows(hwnd, *pResult);
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&result));

    return result;
}

#pragma endregion

#pragma region 按键模拟（纯消息，不占物理键盘）

/// 在 ToDesk 进程内向所有窗口发送按键消息
/// @param vkCode 虚拟键码
static void SimulateKeyPressInProcess(UINT vkCode)
{
    auto allWnds = GetToDeskAllWindows();
    BYTE scanCode = (BYTE)MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);

    for (HWND hWnd : allWnds) {
        if (!IsWindow(hWnd)) continue;
        if (!IsWindowVisible(hWnd)) continue;

        // 发送 WM_KEYDOWN
        SendMessageTimeoutW(hWnd, WM_KEYDOWN, (WPARAM)vkCode,
            MAKELPARAM(1, scanCode), SMTO_NORMAL, 100, nullptr);

        // 发送 WM_CHAR（字符消息）
        SendMessageTimeoutW(hWnd, WM_CHAR, (WPARAM)vkCode,
            MAKELPARAM(1, scanCode), SMTO_NORMAL, 100, nullptr);

        // 发送 WM_KEYUP
        SendMessageTimeoutW(hWnd, WM_KEYUP, (WPARAM)vkCode,
            MAKELPARAM(1, scanCode | 0x80), SMTO_NORMAL, 100, nullptr);
    }
}

/// 在 ToDesk 进程内通过 AttachThreadInput 模拟按键
/// 这种方式让系统认为按键来自 ToDesk 的主线程
static void SimulateKeyPressAttached(UINT vkCode)
{
    // 获取当前进程所有 UI 线程
    DWORD curPid = GetCurrentProcessId();
    DWORD mainTid = 0;
    HWND mainWnd = nullptr;

    // 找到 ToDesk 的主窗口和主线程
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* pParam = reinterpret_cast<std::pair<DWORD, std::pair<DWORD*, HWND*>*>*>(lParam);
        DWORD targetPid = pParam->first;
        auto* pOut = pParam->second;

        DWORD wndPid = 0;
        DWORD wndTid = GetWindowThreadProcessId(hwnd, &wndPid);
        if (wndPid == targetPid) {
            wchar_t cls[128] = {};
            GetClassNameW(hwnd, cls, 128);
            // 找 ToDesk 的主窗口（类名通常含 "ToDesk" 或窗口可见且有标题栏）
            if (wcscmp(cls, L"Qt5152QWindowIcon") == 0 || wcscmp(cls, L"Qt5152QWindow") == 0 ||
                (IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == nullptr &&
                 GetWindowTextLengthW(hwnd) > 0)) {
                *pOut->first = wndTid;
                *pOut->second = hwnd;
                return FALSE;
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(new std::pair<DWORD, std::pair<DWORD*, HWND*>*>(
        curPid, new std::pair<DWORD*, HWND*>(&mainTid, &mainWnd))));

    if (!mainWnd || !mainTid) {
        // 找不到主窗口，降级为直接发消息到所有窗口
        SimulateKeyPressInProcess(vkCode);
        return;
    }

    // 附加到 ToDesk 主线程的输入队列
    DWORD myTid = GetCurrentThreadId();
    BOOL attached = AttachThreadInput(myTid, mainTid, TRUE);

    BYTE scanCode = (BYTE)MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);

    // 发送到主窗口及所有子窗口
    std::vector<HWND> allWnds;
    EnumAllChildWindows(mainWnd, allWnds);

    for (HWND hWnd : allWnds) {
        if (!IsWindow(hWnd)) continue;
        if (!IsWindowVisible(hWnd) && hWnd != mainWnd) continue;

        SendMessageTimeoutW(hWnd, WM_KEYDOWN, (WPARAM)vkCode,
            MAKELPARAM(1, scanCode), SMTO_NORMAL, 100, nullptr);
        SendMessageTimeoutW(hWnd, WM_CHAR, (WPARAM)vkCode,
            MAKELPARAM(1, scanCode), SMTO_NORMAL, 100, nullptr);
        SendMessageTimeoutW(hWnd, WM_KEYUP, (WPARAM)vkCode,
            MAKELPARAM(1, scanCode | 0x80), SMTO_NORMAL, 100, nullptr);
    }

    // 分离线程输入
    if (attached) {
        AttachThreadInput(myTid, mainTid, FALSE);
    }
}

/// 翻页：尝试 AttachThreadInput 方案，失败则降级为直接发消息
static void DoPageTurn(UINT vkCode)
{
    if (vkCode == 0) vkCode = VK_SPACE;

    // 先尝试 AttachThreadInput 方案（模拟来自 ToDesk 主线程的输入）
    SimulateKeyPressAttached(vkCode);
}

#pragma endregion

// 窗口过程
static LRESULT CALLBACK InjectWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_INJECT_PAGE_TURN) {
        // wParam = 要模拟的虚拟键码（如 VK_SPACE, VK_NEXT 等）
        // 在 ToDesk 进程内通过消息模拟按键，不占用物理键盘鼠标
        DoPageTurn((UINT)wParam);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// 创建隐藏窗口用于接收命令
static bool CreateInjectWindow()
{
    WNDCLASSW wc = {};
    wc.lpfnWndProc = InjectWndProc;
    wc.hInstance = g_hInst;
    wc.lpszClassName = L"Shell_TrayWnd_360h_Safe";
    RegisterClassW(&wc);

    g_hWnd = CreateWindowExW(0, wc.lpszClassName, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, g_hInst, nullptr);
    return (g_hWnd != nullptr);
}

extern "C" {
    __declspec(dllexport) bool Inject_Init()
    {
        return CreateInjectWindow();
    }

    __declspec(dllexport) void Inject_Uninit()
    {
        if (g_hWnd) {
            DestroyWindow(g_hWnd);
            g_hWnd = nullptr;
        }
    }

    __declspec(dllexport) HWND Inject_GetWindowHandle()
    {
        return g_hWnd;
    }
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hInst = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        // 注入后立即创建隐藏窗口，这样才能接收 BitGrid 发来的翻页消息
        CreateInjectWindow();
    }
    return TRUE;
}
