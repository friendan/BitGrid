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
#include <fstream>
#include <ctime>

using namespace ezui;

// 日志文件
std::wofstream g_logFile;

void InitLog() {
    g_logFile.open("debug.log", std::ios::out | std::ios::trunc);
    g_logFile << L"=== BitGrid Debug Log Started ===" << std::endl;
}

void Log(const std::wstring& msg) {
    if (g_logFile.is_open()) {
        time_t now = time(0);
        tm ltm;
        localtime_s(&ltm, &now);
        wchar_t timeStr[32];
        swprintf_s(timeStr, L"[%02d:%02d:%02d] ", ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
        g_logFile << timeStr << msg << std::endl;
        g_logFile.flush();
    }
}

void CloseLog() {
    if (g_logFile.is_open()) {
        g_logFile << L"=== Log Ended ===" << std::endl;
        g_logFile.close();
    }
}

class MainFrm : public Window {
private:
    UIManager ui;
    VListView* logList = nullptr;
    Label* statusLeft = nullptr;
    Label* statusCenter = nullptr;
    Label* statusRight = nullptr;

    RECT selectedRectScreen{};
    bool hasSelection = false;
    
public:
    MainFrm(int width, int height) : Window(width, height) {
        Log(L"MainFrm constructor started");
        Init();
        Log(L"MainFrm constructor completed");
    }
    
    void Init() {
        Log(L"Init() started");
        this->SetText(L"BitGrid");
        Log(L"Window title set");
        
        // 从RC资源加载布局
        std::wstring xmlContent;
        Log(L"Trying RC resource...");
        
        HRSRC hRsrc = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_MAIN_LAYOUT), RT_HTML);
        Log(L"FindResourceW RT_HTML result: " + std::to_wstring((ULONG_PTR)hRsrc));
        
        if (!hRsrc) {
            hRsrc = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_MAIN_LAYOUT), L"HTML");
            Log(L"FindResourceW L\"HTML\" result: " + std::to_wstring((ULONG_PTR)hRsrc));
        }
        
        if (hRsrc) {
            HGLOBAL hGlobal = LoadResource(NULL, hRsrc);
            Log(L"LoadResource result: " + std::to_wstring((ULONG_PTR)hGlobal));
            if (hGlobal) {
                DWORD size = SizeofResource(NULL, hRsrc);
                Log(L"Resource size: " + std::to_wstring(size));
                const char* xmlData = (const char*)LockResource(hGlobal);
                if (xmlData && size > 0) {
                    Log(L"Resource locked, converting to wide string");
                    int wideLen = MultiByteToWideChar(CP_UTF8, 0, xmlData, size, NULL, 0);
                    Log(L"Wide char length: " + std::to_wstring(wideLen));
                    if (wideLen > 0) {
                        xmlContent.resize(wideLen);
                        MultiByteToWideChar(CP_UTF8, 0, xmlData, size, &xmlContent[0], wideLen);
                        Log(L"Loaded from RC resource successfully");
                    }
                }
            }
        }
        
        Log(L"XML content size: " + std::to_wstring(xmlContent.size()));
        if (!xmlContent.empty()) {
            Log(L"Calling LoadXmlData...");
            ui.LoadXmlData(xmlContent.c_str());
            Log(L"LoadXmlData completed");
        }
        else {
            Log(L"ERROR: XML content is empty!");
        }
        
        Log(L"Calling SetupUI...");
        ui.SetupUI(this);
        Log(L"SetupUI completed");
        
        Log(L"Getting control references...");
        logList = (VListView*)this->FindControl("logList");
        statusLeft = (Label*)this->FindControl("statusLeft");
        statusCenter = (Label*)this->FindControl("statusCenter");
        statusRight = (Label*)this->FindControl("statusRight");
        
        Log(L"logList: " + std::to_wstring((ULONG_PTR)logList));
        Log(L"statusLeft: " + std::to_wstring((ULONG_PTR)statusLeft));
        Log(L"statusCenter: " + std::to_wstring((ULONG_PTR)statusCenter));
        Log(L"statusRight: " + std::to_wstring((ULONG_PTR)statusRight));
        
        Log(L"Adding startup log...");
        AddLog(L"ready...");
        
        Log(L"Updating status bar...");
        UpdateStatus(L"就绪", L"", L"");
        
        Log(L"Init() completed");
    }
    
    void AddLog(const std::wstring& message) {
        // 同时写入日志文件
        Log(message);
        
        if (logList) {
            // 获取当前时间
            time_t now = time(0);
            tm ltm;
            localtime_s(&ltm, &now);
            wchar_t timeStr[32];
            swprintf_s(timeStr, L"%04d-%02d-%02d %02d:%02d:%02d", 
                ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
                ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
            
            // 组合时间戳和消息
            std::wstring fullMessage = std::wstring(timeStr) + L" " + message;
            
            Label* logItem = new Label();
            logItem->SetText(fullMessage);
            logItem->SetFixedHeight(20);
            logItem->Style.FontSize = 12;
            logItem->TextAlign = TextAlign::TopLeft;  // 左对齐
            logList->Add(logItem);
            logList->Invalidate();
        }
    }
    
    void ClearLog() {
        if (logList) {
            logList->Clear(true);
            logList->Invalidate();
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
                    AddLog(L"[WARN] 请先点击“选择”框选区域");
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
            else if (sender->Name == "btnRecognize") {
                std::wstring dir = PathUtil::GetTodayFolderPath();
                AddLog(L"[INFO] 开始识别目录: " + dir);

                std::string fileName;
                std::string fileContentHex;
                std::string allHex = DrawGrid::RestoreFromFolder(dir, &fileName, &fileContentHex);
                if (allHex.empty() || fileContentHex.empty()) {
                    AddLog(L"[ERROR] 识别失败：未还原到有效数据（请确认目录下存在截图）");
                    UpdateStatus(L"识别失败", L"", L"");
                    return true;
                }

                std::wstring wFileName = AppUtil::StrToWStr(fileName);
                wFileName = PathUtil::SanitizeFileName(wFileName, L"restored.bin");

                std::wstring outPath = PathUtil::GetExeDir() + L"\\" + wFileName;
                if (!AppUtil::WriteHexStringToFile(fileContentHex, outPath)) {
                    AddLog(L"[ERROR] 写入还原文件失败: " + outPath);
                    UpdateStatus(L"识别失败", L"", L"");
                    return true;
                }

                AddLog(L"[INFO] 识别成功，已生成文件: " + outPath);
                UpdateStatus(L"识别成功", L"", L"");
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
            }
            else if (sender->Name == "btnAbout") {
                MessageBoxW(this->Hwnd(), L"BitGrid", L"关于", MB_OK);
            }
        }
        return __super::OnNotify(sender, args);
    }
    
    virtual void OnClose(bool& close) override {
        Application::Exit(0);
    }
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    InitLog();
    Log(L"=== Program Started ===");
    
    // 初始化GDI+
    DrawGrid::Inst()->InitGdiPlus();
    Log(L"GDI+ initialized");
    
    Application app;
    app.EnableHighDpi();
    
    MainFrm frm(900, 600);
    frm.CenterToScreen();
    frm.Show();
    
    int result = app.Exec();
    
    // 卸载GDI+
    DrawGrid::Inst()->UninitGdiPlus();
    Log(L"GDI+ shutdown");
    
    CloseLog();
    return result;
}
