#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <cstdint>

/// 注入 / 卸载辅助：将 InjectDll.dll 注入 ToDesk_Client.exe
/// 处理两个同进程名的情况：两个进程都注入，但只跟有可见窗口的那个通信
class InjectHelper
{
public:
    InjectHelper() = default;
    ~InjectHelper() { Uninit(); }

    /// 查找所有 ToDesk_Client.exe 进程并注入 DLL
    /// @param dllPath  InjectDll.dll 的完整路径
    /// @return 成功（至少注入了一个有可见窗口的进程）返回 true
    bool Init(const std::wstring& dllPath)
    {
        if (m_hInjectWnd) return true; // 已经注入

        // 1. 查找所有 ToDesk_Client.exe 进程
        std::vector<DWORD> pids = FindAllProcessIds(L"ToDesk_Client.exe");
        if (pids.empty()) {
            m_lastError = L"未找到 ToDesk_Client.exe 进程";
            return false;
        }

        // 2. 对每个进程尝试注入
        for (DWORD pid : pids) {
            if (InjectSingleProcess(pid, dllPath)) {
                // 注入成功，查找本进程中 DLL 创建的隐藏窗口
                if (FindInjectWindow(pid)) {
                    // 找到了隐藏窗口，再确认这个进程有可见的顶层窗口
                    // （有可见窗口的才是负责远程桌面的主进程）
                    DWORD wndPid = 0;
                    GetWindowThreadProcessId(m_hInjectWnd, &wndPid);
                    if (HasVisibleTopWindow(wndPid)) {
                        m_lastError = L"";
                        return true; // 找到主进程，注入完成
                    }
                    // 这个进程没有可见窗口（是后台进程），继续看下一个
                    m_hInjectWnd = nullptr;
                    m_hDllModule = nullptr;
                }
            }
        }

        // 3. 如果所有进程都注入了但没找到有可见窗口的，保留最后一个注入的
        //    （可能是 ToDesk 窗口还没创建/不可见）
        if (!m_pids.empty()) {
            m_lastError = L"已注入所有进程但未检测到可见窗口，翻页可能不生效";
            return true;
        }

        m_lastError = L"注入所有 ToDesk 进程均失败";
        return false;
    }

    /// 发送翻页消息
    /// @param vkCode 虚拟键码（默认 VK_SPACE）
    void SendPageTurn(UINT vkCode = VK_SPACE)
    {
        // 向所有已注入的进程发送翻页消息（翻页时只作用主窗口所在进程）
        if (m_hInjectWnd) {
            PostMessage(m_hInjectWnd, WM_INJECT_PAGE_TURN, (WPARAM)vkCode, 0);
        }
    }

    /// 卸载注入（所有进程）
    void Uninit()
    {
        // 向每个已注入的进程发送 FreeLibrary 卸载
        for (auto& entry : m_injectedProcesses) {
            if (entry.hProcess && entry.hDllModule) {
                LPTHREAD_START_ROUTINE freeLibAddr = (LPTHREAD_START_ROUTINE)
                    GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "FreeLibrary");
                if (freeLibAddr) {
                    HANDLE hThread = CreateRemoteThread(entry.hProcess, nullptr, 0,
                        freeLibAddr, entry.hDllModule, 0, nullptr);
                    if (hThread) {
                        WaitForSingleObject(hThread, 3000);
                        CloseHandle(hThread);
                    }
                }
                CloseHandle(entry.hProcess);
            }
        }
        m_injectedProcesses.clear();
        m_pids.clear();
        m_hInjectWnd = nullptr;
        m_hDllModule = nullptr;
    }

    /// 是否已注入成功
    bool IsInjected() const { return m_hInjectWnd != nullptr; }

    /// 获取最后错误信息
    std::wstring GetLastError() const { return m_lastError; }

private:
    struct InjectEntry {
        DWORD  pid;
        HANDLE hProcess;
        HMODULE hDllModule;
    };

    std::vector<InjectEntry> m_injectedProcesses;
    std::vector<DWORD> m_pids;
    HWND   m_hInjectWnd   = nullptr;
    HMODULE m_hDllModule  = nullptr; // 主进程 DLL 模块基址
    std::wstring m_lastError;

    // 查找所有匹配进程名的 PID
    static std::vector<DWORD> FindAllProcessIds(const std::wstring& processName)
    {
        std::vector<DWORD> pids;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return pids;

        PROCESSENTRY32W pe = { sizeof(PROCESSENTRY32W) };
        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, processName.c_str()) == 0) {
                    pids.push_back(pe.th32ProcessID);
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
        return pids;
    }

    // 注入单个进程
    bool InjectSingleProcess(DWORD pid, const std::wstring& dllPath)
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (!hProcess) {
            m_lastError = L"无法打开进程 (pid:" + std::to_wstring(pid) + L")，请以管理员身份运行";
            return false;
        }

        // 在目标进程分配内存，写入 DLL 路径
        size_t pathSize = (dllPath.size() + 1) * sizeof(wchar_t);
        void* remoteMem = VirtualAllocEx(hProcess, nullptr, pathSize, MEM_COMMIT, PAGE_READWRITE);
        if (!remoteMem) {
            CloseHandle(hProcess);
            return false;
        }

        if (!WriteProcessMemory(hProcess, remoteMem, dllPath.c_str(), pathSize, nullptr)) {
            VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        // 创建远程线程加载 DLL
        LPTHREAD_START_ROUTINE loadLibAddr = (LPTHREAD_START_ROUTINE)
            GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
        if (!loadLibAddr) {
            VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
            loadLibAddr, remoteMem, 0, nullptr);
        if (!hThread) {
            VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        WaitForSingleObject(hThread, 5000);
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);

        // 保存注入信息
        InjectEntry entry;
        entry.pid = pid;
        entry.hProcess = hProcess;
        entry.hDllModule = nullptr; // 后面查找
        m_injectedProcesses.push_back(entry);
        m_pids.push_back(pid);

        return true;
    }

    // 通过枚举窗口找到目标进程中的 Inject 隐藏窗口
    bool FindInjectWindow(DWORD targetPid)
    {
        struct EnumCtx { DWORD pid; HWND hwnd; };
        EnumCtx ctx = { targetPid, nullptr };

        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            auto* ctx = reinterpret_cast<EnumCtx*>(lParam);
            DWORD wndPid = 0;
            GetWindowThreadProcessId(hwnd, &wndPid);
            if (wndPid == ctx->pid) {
                wchar_t className[256];
                if (GetClassNameW(hwnd, className, 256)) {
                    if (wcscmp(className, L"BitGridInjectClass") == 0) {
                        ctx->hwnd = hwnd;
                        return FALSE;
                    }
                }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&ctx));

        if (ctx.hwnd) {
            m_hInjectWnd = ctx.hwnd;
            // 获取 DLL 模块基址
            GetWindowThreadProcessId(m_hInjectWnd, &targetPid);
            HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, targetPid);
            if (hSnapshot != INVALID_HANDLE_VALUE) {
                MODULEENTRY32W me = { sizeof(MODULEENTRY32W) };
                if (Module32FirstW(hSnapshot, &me)) {
                    do {
                        std::wstring modName = me.szModule;
                        if (modName.find(L"360safe") != std::wstring::npos ||
                            modName.find(L"360safe_dbg") != std::wstring::npos) {
                            m_hDllModule = me.hModule;
                            // 更新 injection entry 中的模块基址
                            for (auto& entry : m_injectedProcesses) {
                                if (entry.pid == targetPid) {
                                    entry.hDllModule = me.hModule;
                                    break;
                                }
                            }
                            break;
                        }
                    } while (Module32NextW(hSnapshot, &me));
                }
                CloseHandle(hSnapshot);
            }
            return true;
        }
        return false;
    }

    // 判断指定进程是否有可见的顶层窗口
    static bool HasVisibleTopWindow(DWORD pid)
    {
        struct Ctx { DWORD pid; bool found; };
        Ctx ctx = { pid, false };
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            auto* ctx = reinterpret_cast<Ctx*>(lParam);
            DWORD wndPid = 0;
            GetWindowThreadProcessId(hwnd, &wndPid);
            if (wndPid == ctx->pid) {
                if (IsWindowVisible(hwnd)) {
                    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
                    if (style & WS_CAPTION) {
                        ctx->found = true;
                        return FALSE;
                    }
                }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&ctx));

        return ctx.found;
    }
};
