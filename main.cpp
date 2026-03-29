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
    
public:
    MainFrm(int width, int height) : Window(width, height) {
        Log(L"MainFrm constructor started");
        Init();
        Log(L"MainFrm constructor completed");
    }
    
    void Init() {
        Log(L"Init() started");
        this->SetText(L"BitGrid - 日志查看器");
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
        UpdateStatus(L"就绪", L"", L"2024-01-01 12:00:00");
        
        Log(L"Init() completed");
    }
    
    void AddLog(const std::wstring& message) {
        if (logList) {
            Label* logItem = new Label();
            logItem->SetText(message);
            logItem->SetFixedHeight(20);
            logItem->Style.FontSize = 12;
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
            if (sender->Name == "btnStart") {
                AddLog(L"[INFO] 开始执行任务...");
                UpdateStatus(L"运行中", L"处理中...", L"");
            }
            else if (sender->Name == "btnStop") {
                AddLog(L"[WARN] 任务已停止");
                UpdateStatus(L"已停止", L"", L"");
            }
            else if (sender->Name == "btnClear") {
                ClearLog();
                AddLog(L"[INFO] 日志已清空");
            }
            else if (sender->Name == "btnSave") {
                AddLog(L"[INFO] 日志保存成功");
            }
            else if (sender->Name == "btnSettings") {
                AddLog(L"[INFO] 打开设置对话框");
            }
            else if (sender->Name == "btnAbout") {
                MessageBoxW(this->Hwnd(), L"BitGrid v1.0\n基于ezui的日志查看器", L"关于", MB_OK);
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
    
    Application app;
    app.EnableHighDpi();
    
    MainFrm frm(900, 600);
    frm.CenterToScreen();
    frm.Show();
    
    int result = app.Exec();
    CloseLog();
    return result;
}
