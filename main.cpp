#include <Windows.h>
#include <Application.h>
#include <Window.h>
#include <VLayout.h>
#include <HLayout.h>
#include <Label.h>
#include <Button.h>
#include <TextBox.h>
#include <CheckBox.h>
#include <VListView.h>
#include <UIManager.h>
#include "resource.h"
#include "DrawGrid.hpp"
#include "AppUtil.hpp"
#include "PathUtil.hpp"
#include "ScreenSelectOverlay.hpp"
#include "ScreenCapture.hpp"
#include "MnnOcr.hpp"
#include <fstream>
#include <ctime>
#include <thread>
#include <atomic>

using namespace ezui;

class MainFrm : public Window {
private:
    UIManager ui;
    TextBox* logBox = nullptr;  // 使用 TextBox，支持文本选择和复制
    Label* statusLeft = nullptr;
    Label* statusCenter = nullptr;
    Label* statusRight = nullptr;
    Button* btnRecognize = nullptr;  // 识别按钮指针

    RECT selectedRectScreen{};
    bool hasSelection = false;
    std::atomic<bool> isRecognizing{false};  // 是否正在识别
    std::atomic<bool> isAutoActionRunning{false};  // 自动操作是否正在运行
    MnnOcr m_ocr;  // OCR 引擎
    
public:
    MainFrm(int width, int height) : Window(width, height) {
        // AppUtil::SaveLog("[BitGrid] MainFrm constructor started");
        Init();
        // AppUtil::SaveLog("[BitGrid] MainFrm constructor completed");
    }
    
    /// 获取总页数输入框的值
    int GetTotalPage() {
        auto* txt = (TextBox*)this->FindControl("txtTotalPage");
        if (!txt) return 1;
        std::wstring text = txt->GetText().unicode();
        if (text.empty()) return 1;
        try {
            return std::stoi(text);
        } catch (...) {
            return 1;
        }
    }

    void Init() {
        this->SetText(L"BitGrid");
        
        // 从RC资源加载布局
        std::wstring xmlContent;
        
        HRSRC hRsrc = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_MAIN_LAYOUT), RT_HTML);
        
        if (!hRsrc) {
            hRsrc = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_MAIN_LAYOUT), L"HTML");
        }
        
        if (hRsrc) {
            HGLOBAL hGlobal = LoadResource(NULL, hRsrc);
            if (hGlobal) {
                DWORD size = SizeofResource(NULL, hRsrc);
                const char* xmlData = (const char*)LockResource(hGlobal);
                if (xmlData && size > 0) {
                    int wideLen = MultiByteToWideChar(CP_UTF8, 0, xmlData, size, NULL, 0);
                    if (wideLen > 0) {
                        xmlContent.resize(wideLen);
                        MultiByteToWideChar(CP_UTF8, 0, xmlData, size, &xmlContent[0], wideLen);
                    }
                }
            }
        }
        
        if (!xmlContent.empty()) {
            ui.LoadXmlData(xmlContent.c_str());
        }
        
        ui.SetupUI(this);
        
        logBox = (TextBox*)this->FindControl("logBox");
        statusLeft = (Label*)this->FindControl("statusLeft");
        statusCenter = (Label*)this->FindControl("statusCenter");
        statusRight = (Label*)this->FindControl("statusRight");
        btnRecognize = (Button*)this->FindControl("btnRecognize");
        
        if (logBox) {
            logBox->Style.FontSize = 12;
        }
        
        AddLog(L"ready...");
        
        // 初始化 OCR 引擎
        if (!m_ocr.load() || !m_ocr.init(4)) {
            AppUtil::SaveLog("[MainFrm] OCR engine init failed");
        }
        
        // 注册全局快捷键（Alt+F6/F7/F8 停止自动操作）
        RegisterHotKey(this->Hwnd(), 100, MOD_ALT, VK_F6);
        RegisterHotKey(this->Hwnd(), 101, MOD_ALT, VK_F7);
        RegisterHotKey(this->Hwnd(), 102, MOD_ALT, VK_F8);
        
        AddLog(L"[INFO] Alt+F6/F7/F8 可停止自动操作");
        
        UpdateStatus(L"就绪", L"", L"");
    }
    
    // ---------- 自动操作相关 ----------
    
    /// 对指定区域截图并保存到文件
    std::wstring CaptureToFile(const RECT& rect, const std::wstring& dir) {
        std::wstring outPng = PathUtil::NextPngPathInDir(dir);
        std::wstring err;
        if (!ScreenCapture::CaptureRectToPng(rect, outPng, &err)) {
            PostLog(L"[ERROR] 截图失败: " + err);
            return L"";
        }
        return outPng;
    }
    
    /// 在后台线程中安全地发送日志到 UI 线程
    void PostLog(const std::wstring& msg) {
        PostMessage(this->Hwnd(), WM_USER + 101, 0,
            reinterpret_cast<LPARAM>(new std::wstring(msg)));
    }
    
    /// 模拟按键（keybd_event + 正确 scan code）
    void SimulateKeyPress(int vkKey) {
        BYTE scanCode = (BYTE)MapVirtualKeyA(vkKey, MAPVK_VK_TO_VSC);
        // 按键按下
        keybd_event((BYTE)vkKey, scanCode, 0, 0);
        Sleep(50);
        // 按键弹起
        keybd_event((BYTE)vkKey, scanCode, KEYEVENTF_KEYUP, 0);
        Sleep(2000);  // 等待翻页完成
    }
    
    /// 将鼠标移动到指定屏幕坐标
    void SimulateMouseMove(int x, int y) {
        SetCursorPos(x, y);
        Sleep(100);
    }
    
    /// 模拟鼠标左键单击
    void SimulateMouseClick() {
        // 鼠标左键按下
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        Sleep(50);
        // 鼠标左键弹起
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        Sleep(300);
    }

    /// 自动操作：循环截图 + 翻页 + 触发识别
    void RunAutoAction() {
        int totalPage = GetTotalPage();
        PostLog(L"[INFO] 总页数: " + std::to_wstring(totalPage));
        
        // 清空并创建截图目录
        std::wstring dir = PathUtil::GetTodayFolderPath();
        PathUtil::RemoveDirRecursive(dir);
        PathUtil::EnsureDirExists(dir);
        
        int centerX = (selectedRectScreen.left + selectedRectScreen.right) / 2;
        int centerY = (selectedRectScreen.top + selectedRectScreen.bottom) / 2;
        
        for (int page = 1; page <= totalPage; page++) {
            // 检查是否被中断
            if (!isAutoActionRunning.load()) {
                PostLog(L"[INFO] 用户中断自动操作");
                return;
            }
            
            // 截图
            std::wstring pngPath = CaptureToFile(selectedRectScreen, dir);
            if (pngPath.empty()) {
                PostLog(L"[ERROR] 截图失败，流程终止");
                FinishAutoAction(false);
                return;
            }
            PostLog(L"[INFO] 已截图(" + std::to_wstring(page) + L"/" + std::to_wstring(totalPage) + L"): " + pngPath);
            
            // 最后一页不翻页
            if (page >= totalPage) break;
            
            // 翻页：来回移动鼠标防止空闲（只移动不单击）
            for (int i = -20; i <= 20; i += 2) {
                SetCursorPos(centerX + i, centerY);
                Sleep(5);
            }
            for (int i = 20; i >= -20; i -= 2) {
                SetCursorPos(centerX + i, centerY);
                Sleep(5);
            }
            // 单击激活窗口
            SimulateMouseClick();
            Sleep(100);
            SimulateKeyPress(VK_SPACE);
        }
        
        // 鼠标移回自动操作按钮
        POINT btnPos = FindAutoActionBtnScreenPos();
        if (btnPos.x != 0 || btnPos.y != 0) {
            SimulateMouseMove(btnPos.x, btnPos.y);
        }
        
        // 触发手动识别
        PostLog(L"[INFO] 截图完成，触发识别");
        PostMessage(this->Hwnd(), WM_USER + 200, 0, 0);
    }
    
    /// 查找自动操作按钮的屏幕坐标
    POINT FindAutoActionBtnScreenPos() {
        POINT pt = { 0, 0 };
        auto* btn = this->FindControl("btnAutoAction");
        if (btn) {
            auto& r = btn->GetRect();
            pt.x = r.X;
            pt.y = r.Y;
            ClientToScreen(this->Hwnd(), &pt);
            pt.x += r.Width / 2;
            pt.y += r.Height / 2;
        }
        return pt;
    }
    
    /// 结束自动操作（在后台线程中调用）
    void FinishAutoAction(bool success) {
        PostMessage(this->Hwnd(), WM_USER + 201, success ? 1 : 0, 0);
    }
    
    // ---------- 日志 ----------
    
    void AddLog(const std::wstring& message) {
        // 只打印到窗口，不写日志文件
        // AppUtil::SaveLog("[BitGrid UI] ", AppUtil::WStrToStr(message));
        
        if (logBox) {
            // 获取当前时间
            time_t now = time(0);
            tm ltm;
            localtime_s(&ltm, &now);
            wchar_t timeStr[32];
            swprintf_s(timeStr, L"%04d-%02d-%02d %02d:%02d:%02d", 
                ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
                ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
            
            // 组合时间戳和消息
            std::wstring fullMessage = std::wstring(timeStr) + L" " + message + L"\r\n";
            
            // 获取当前文本并追加新日志
            std::wstring currentText = AppUtil::StrToWStr(logBox->GetText().c_str());
            logBox->SetText((currentText + fullMessage).c_str());
        }
    }
    
    void ClearLog() {
        if (logBox) {
            logBox->SetText(L"");
        }
    }
    
    void UpdateStatus(const std::wstring& left, const std::wstring& center, const std::wstring& right) {
        if (statusLeft) statusLeft->SetText(left);
        if (statusCenter) statusCenter->SetText(center);
        if (statusRight) statusRight->SetText(right);
    }
    
    virtual bool OnNotify(Control* sender, EventArgs& args) override {
        if (args.EventType == Event::OnMouseDown) {
            if (sender->Name == "btnSelect") {
                AddLog(L"[INFO] 进入选择模式：拖拽框选区域，ESC/右键取消");
                RECT rc{};
                if (ScreenSelectOverlay::SelectRect(rc)) {
                    selectedRectScreen = rc;
                    hasSelection = true;
                    AddLog(L"[INFO] 已选择区域: (" + std::to_wstring(rc.left) + L"," + std::to_wstring(rc.top) + L")-(" +
                        std::to_wstring(rc.right) + L"," + std::to_wstring(rc.bottom) + L")");
                    UpdateStatus(L"已选择", L"", L"");
                }
                else {
                    AddLog(L"[WARN] 已取消选择");
                    UpdateStatus(L"就绪", L"", L"");
                }
            }
            else if (sender->Name == "btnShot") {
                if (!hasSelection) {
                    AddLog(L"[WARN] 请先点击\"选择\"框选区域");
                    return true;
                }
                std::wstring dir = PathUtil::GetTodayFolderPath();
                if (!PathUtil::EnsureDirExists(dir)) {
                    AddLog(L"[ERROR] 创建目录失败: " + dir);
                    return true;
                }
                std::wstring outPng = PathUtil::NextPngPathInDir(dir);
                std::wstring err;
                if (!ScreenCapture::CaptureRectToPng(selectedRectScreen, outPng, &err)) {
                    AddLog(L"[ERROR] 截图失败: " + err);
                    return true;
                }
                AddLog(L"[INFO] 截图已保存: " + outPng);
                UpdateStatus(L"已截图", L"", L"");
            }
            else if (sender->Name == "btnAutoAction") {
                if (isAutoActionRunning.load()) {
                    AddLog(L"[WARN] 自动操作正在进行中，请稍候...");
                    return true;
                }
                if (!hasSelection) {
                    AddLog(L"[WARN] 请先选择【窗口区域】");
                    return true;
                }
                
                isAutoActionRunning.store(true);
                sender->SetEnabled(false);
                
                // 启动后台线程执行自动操作
                std::thread([this]() {
                    RunAutoAction();
                }).detach();
            }
            else if (sender->Name == "btnRecognize") {
                // 如果正在识别，忽略点击
                if (isRecognizing.load()) {
                    AddLog(L"[WARN] 识别正在进行中，请稍候...");
                    return true;
                }
                
                // 设置识别状态
                isRecognizing.store(true);
                
                // 禁用识别按钮
                if (btnRecognize) {
                    btnRecognize->SetEnabled(false);
                }
                UpdateStatus(L"识别中...", L"", L"");
                AddLog(L"[INFO] 开始识别目录: " + PathUtil::GetTodayFolderPath());
                
                // 在后台线程中执行识别
                std::thread([this]() {
                    std::wstring dir = PathUtil::GetTodayFolderPath();
                    
                    std::string fileName;
                    std::string fileContentHex;
                    
                    // 定义进度回调函数
                    auto progressCallback = [this](int current, int total, const std::wstring& filePath) {
                        // 提取文件名
                        size_t lastSlash = filePath.find_last_of(L"\\");
                        std::wstring fileName = (lastSlash != std::wstring::npos) ? 
                            filePath.substr(lastSlash + 1) : filePath;
                        
                        // 构建进度消息
                        wchar_t progressMsg[512];
                        swprintf_s(progressMsg, L"[INFO] 正在识别: %s (%d/%d)", 
                            fileName.c_str(), current, total);
                        
                        // 发送消息到UI线程更新日志
                        PostMessage(this->Hwnd(), WM_USER + 101, 0,
                            reinterpret_cast<LPARAM>(new std::wstring(progressMsg)));
                    };
                    
                    std::string allHex = DrawGrid::RestoreFromFolder(dir, &fileName, &fileContentHex, progressCallback);
                    
                    // 识别完成，回到UI线程更新界面
                    PostMessage(this->Hwnd(), WM_USER + 100, 
                        allHex.empty() || fileContentHex.empty() ? 0 : 1,
                        reinterpret_cast<LPARAM>(new std::pair<std::string, std::string>(fileName, fileContentHex)));
                }).detach();
            }
            else if (sender->Name == "btnClean") {
                std::wstring dir = PathUtil::GetTodayFolderPath();
                std::wstring err;
                if (!PathUtil::RemoveDirRecursive(dir, &err)) {
                    AddLog(L"[ERROR] 清理失败: " + dir + L" (" + err + L")");
                    UpdateStatus(L"清理失败", L"", L"");
                    return true;
                }
                AddLog(L"[INFO] 已清理目录: " + dir);
                UpdateStatus(L"已清理", L"", L"");
                ClearLog();  // 清空日志窗口
                // 重置状态
                isAutoActionRunning.store(false);
                isRecognizing.store(false);
                auto* btnAutoAction = this->FindControl("btnAutoAction");
                if (btnAutoAction) btnAutoAction->SetEnabled(true);
                if (btnRecognize) btnRecognize->SetEnabled(true);
            }
            else if (sender->Name == "btnAbout") {
                MessageBoxW(this->Hwnd(), L"BitGrid", L"关于", MB_OK);
            }
        }
        return __super::OnNotify(sender, args);
    }
    
    // 处理自定义消息（识别完成）
    virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam) override {
        if (msg == WM_USER + 100) {
            // 识别完成消息
            bool success = (wParam == 1);
            auto* pData = reinterpret_cast<std::pair<std::string, std::string>*>(lParam);
            
            if (success && pData) {
                std::string fileName = pData->first;
                std::string fileContentHex = pData->second;
                
                std::wstring wFileName = AppUtil::StrToWStr(fileName);
                wFileName = PathUtil::SanitizeFileName(wFileName, L"restored.bin");

                // 在 exe 目录下创建 file 子目录
                std::wstring fileDir = PathUtil::GetExeDir() + L"\\file";
                PathUtil::EnsureDirExists(fileDir);
                
                std::wstring outPath = fileDir + L"\\" + wFileName;
                if (AppUtil::WriteHexStringToFile(fileContentHex, outPath)) {
                    AddLog(L"[INFO] 识别成功，已生成文件: " + outPath);
                    UpdateStatus(L"识别成功", L"", L"");
                } else {
                    AddLog(L"[ERROR] 写入还原文件失败: " + outPath);
                    UpdateStatus(L"识别失败", L"", L"");
                }
            } else {
                AddLog(L"[ERROR] 识别失败：未还原到有效数据（请确认目录下存在截图）");
                UpdateStatus(L"识别失败", L"", L"");
            }
            
            // 释放数据
            delete pData;
            
            // 恢复识别状态
            isRecognizing.store(false);
            if (btnRecognize) {
                btnRecognize->SetEnabled(true);
            }
            
            return 0;
        }
        else if (msg == WM_USER + 101) {
            // 进度更新消息
            auto* pMsg = reinterpret_cast<std::wstring*>(lParam);
            if (pMsg) {
                AddLog(*pMsg);
                delete pMsg;
            }
            return 0;
        }
        else if (msg == WM_USER + 200) {
            // 自动操作截图完成，触发手动识别
            AddLog(L"[INFO] ===== 自动触发手动识别 =====");
            // 模拟点击手动识别按钮
            if (btnRecognize) {
                // 直接调用识别逻辑
                if (!isRecognizing.load()) {
                    isRecognizing.store(true);
                    btnRecognize->SetEnabled(false);
                    UpdateStatus(L"识别中...", L"", L"");
                    AddLog(L"[INFO] 开始识别目录: " + PathUtil::GetTodayFolderPath());
                    
                    std::thread([this]() {
                        std::wstring dir = PathUtil::GetTodayFolderPath();
                        std::string fileName;
                        std::string fileContentHex;
                        
                        auto progressCallback = [this](int current, int total, const std::wstring& filePath) {
                            size_t lastSlash = filePath.find_last_of(L"\\");
                            std::wstring fileName = (lastSlash != std::wstring::npos) ? 
                                filePath.substr(lastSlash + 1) : filePath;
                            wchar_t progressMsg[512];
                            swprintf_s(progressMsg, L"[INFO] 正在识别: %s (%d/%d)", 
                                fileName.c_str(), current, total);
                            PostMessage(this->Hwnd(), WM_USER + 101, 0,
                                reinterpret_cast<LPARAM>(new std::wstring(progressMsg)));
                        };
                        
                        std::string allHex = DrawGrid::RestoreFromFolder(dir, &fileName, &fileContentHex, progressCallback);
                        
                        PostMessage(this->Hwnd(), WM_USER + 100, 
                            allHex.empty() || fileContentHex.empty() ? 0 : 1,
                            reinterpret_cast<LPARAM>(new std::pair<std::string, std::string>(fileName, fileContentHex)));
                    }).detach();
                }
            }
            return 0;
        }
        else if (msg == WM_USER + 201) {
            // 自动操作结束
            bool success = (wParam == 1);
            isAutoActionRunning.store(false);
            auto* btnAutoAction = this->FindControl("btnAutoAction");
            if (btnAutoAction) {
                btnAutoAction->SetEnabled(true);
            }
            if (success) {
                UpdateStatus(L"自动完成", L"", L"");
            } else {
                UpdateStatus(L"自动终止", L"", L"");
            }
            return 0;
        }
        else if (msg == WM_HOTKEY) {
            // Alt+F6 / F7 / F8 停止自动操作
            if (wParam >= 100 && wParam <= 102) {
                if (isAutoActionRunning.load()) {
                    isAutoActionRunning.store(false);
                    auto* btnAutoAction = this->FindControl("btnAutoAction");
                    if (btnAutoAction) btnAutoAction->SetEnabled(true);
                    AddLog(L"[INFO] 用户中断，自动操作已停止");
                    UpdateStatus(L"已中断", L"", L"");
                }
            }
            return 0;
        }
        
        return __super::WndProc(msg, wParam, lParam);
    }
    
    virtual void OnClose(bool& close) override {
        Application::Exit(0);
    }
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // 初始化GDI+
    DrawGrid::Inst()->InitGdiPlus();
    
    Application app;
    app.EnableHighDpi();
    
    MainFrm frm(900, 600);
    frm.SetIcon(IDI_APP_ICON);  // 设置窗口图标
    frm.CenterToScreen();
    frm.Show();
    
    int result = app.Exec();
    
    // 卸载GDI+
    DrawGrid::Inst()->UninitGdiPlus();
    
    return result;
}
