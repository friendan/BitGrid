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
#include <Resource.h>
#include "resource.h"

using namespace ezui;

class MainFrm : public Window {
private:
    UIManager ui;
    VListView* logList = nullptr;
    Label* statusLeft = nullptr;
    Label* statusCenter = nullptr;
    Label* statusRight = nullptr;
    
public:
    MainFrm(int width, int height) : Window(width, height) {
        Init();
    }
    
    void Init() {
        this->SetText(L"BitGrid - 日志查看器");
        
        // 从RC资源加载布局
        HRSRC hRsrc = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_MAIN_LAYOUT), L"HTML");
        if (hRsrc) {
            HGLOBAL hGlobal = LoadResource(NULL, hRsrc);
            if (hGlobal) {
                DWORD size = SizeofResource(NULL, hRsrc);
                const wchar_t* xmlData = (const wchar_t*)LockResource(hGlobal);
                if (xmlData && size > 0) {
                    // 注意：资源数据可能需要BOM处理，这里假设是UTF-16
                    ui.LoadXmlData(xmlData);
                }
            }
        }
        
        ui.SetupUI(this);
        
        // 获取控件引用
        logList = (VListView*)this->FindControl("logList");
        statusLeft = (Label*)this->FindControl("statusLeft");
        statusCenter = (Label*)this->FindControl("statusCenter");
        statusRight = (Label*)this->FindControl("statusRight");
        
        // 添加启动日志
        AddLog(L"ready...");
        
        // 更新状态栏
        UpdateStatus(L"就绪", L"", L"2024-01-01 12:00:00");
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
    Application app;
    app.EnableHighDpi();
    
    MainFrm frm(900, 600);
    frm.CenterToScreen();
    frm.Show();
    
    return app.Exec();
}
