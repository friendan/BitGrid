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

class MainFrm : public Window {
private:
    UIManager ui;
    TextBox* logBox = nullptr;  // 使用 TextBox，支持文本选择和复制
    Label* statusLeft = nullptr;
    Label* statusCenter = nullptr;
    Label* statusRight = nullptr;

    RECT selectedRectScreen{};
    bool hasSelection = false;
    
public:
    MainFrm(int width, int height) : Window(width, height) {
        AppUtil::SaveLog("[BitGrid] MainFrm constructor started");
        Init();
        AppUtil::SaveLog("[BitGrid] MainFrm constructor completed");
    }
    
    void Init() {
        AppUtil::SaveLog("[BitGrid] Init() started");
        this->SetText(L"BitGrid");
        AppUtil::SaveLog("[BitGrid] Window title set");
        
        // 从RC资源加载布局
        std::wstring xmlContent;
        AppUtil::SaveLog("[BitGrid] Trying RC resource...");
        
        HRSRC hRsrc = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_MAIN_LAYOUT), RT_HTML);
        AppUtil::SaveLog("[BitGrid] FindResourceW RT_HTML result: ", std::to_string((ULONG_PTR)hRsrc));
        
        if (!hRsrc) {
            hRsrc = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_MAIN_LAYOUT), L"HTML");
            AppUtil::SaveLog("[BitGrid] FindResourceW L\"HTML\" result: ", std::to_string((ULONG_PTR)hRsrc));
        }
        
        if (hRsrc) {
            HGLOBAL hGlobal = LoadResource(NULL, hRsrc);
            AppUtil::SaveLog("[BitGrid] LoadResource result: ", std::to_string((ULONG_PTR)hGlobal));
            if (hGlobal) {
                DWORD size = SizeofResource(NULL, hRsrc);
                AppUtil::SaveLog("[BitGrid] Resource size: ", std::to_string(size));
                const char* xmlData = (const char*)LockResource(hGlobal);
                if (xmlData && size > 0) {
                    AppUtil::SaveLog("[BitGrid] Resource locked, converting to wide string");
                    int wideLen = MultiByteToWideChar(CP_UTF8, 0, xmlData, size, NULL, 0);
                    AppUtil::SaveLog("[BitGrid] Wide char length: ", std::to_string(wideLen));
                    if (wideLen > 0) {
                        xmlContent.resize(wideLen);
                        MultiByteToWideChar(CP_UTF8, 0, xmlData, size, &xmlContent[0], wideLen);
                        AppUtil::SaveLog("[BitGrid] Loaded from RC resource successfully");
                    }
                }
            }
        }
        
        AppUtil::SaveLog("[BitGrid] XML content size: ", std::to_string(xmlContent.size()));
        if (!xmlContent.empty()) {
            AppUtil::SaveLog("[BitGrid] Calling LoadXmlData...");
            ui.LoadXmlData(xmlContent.c_str());
            AppUtil::SaveLog("[BitGrid] LoadXmlData completed");
        }
        else {
            AppUtil::SaveLog("[BitGrid] ERROR: XML content is empty!");
        }
        
        AppUtil::SaveLog("[BitGrid] Calling SetupUI...");
        ui.SetupUI(this);
        AppUtil::SaveLog("[BitGrid] SetupUI completed");
        
        AppUtil::SaveLog("[BitGrid] Getting control references...");
        logBox = (TextBox*)this->FindControl("logBox");
        statusLeft = (Label*)this->FindControl("statusLeft");
        statusCenter = (Label*)this->FindControl("statusCenter");
        statusRight = (Label*)this->FindControl("statusRight");
        
        AppUtil::SaveLog("[BitGrid] logBox: ", std::to_string((ULONG_PTR)logBox));
        AppUtil::SaveLog("[BitGrid] statusLeft: ", std::to_string((ULONG_PTR)statusLeft));
        AppUtil::SaveLog("[BitGrid] statusCenter: ", std::to_string((ULONG_PTR)statusCenter));
        AppUtil::SaveLog("[BitGrid] statusRight: ", std::to_string((ULONG_PTR)statusRight));
        
        if (logBox) {
            logBox->Style.FontSize = 12;
        }
        
        AppUtil::SaveLog("[BitGrid] Adding startup log...");
        AddLog(L"ready...");
        
        AppUtil::SaveLog("[BitGrid] Updating status bar...");
        UpdateStatus(L"就绪", L"", L"");
        
        AppUtil::SaveLog("[BitGrid] Init() completed");
    }
    
    void AddLog(const std::wstring& message) {
        // 使用 AppUtil::SaveLog 统一记录日志
        AppUtil::SaveLog("[BitGrid UI] ", AppUtil::WStrToStr(message));
        
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
    AppUtil::SaveLog("=== BitGrid Program Started ===");
    
    // 初始化GDI+
    DrawGrid::Inst()->InitGdiPlus();
    AppUtil::SaveLog("[BitGrid] GDI+ initialized");
    
    Application app;
    app.EnableHighDpi();
    
    MainFrm frm(900, 600);
    frm.CenterToScreen();
    frm.Show();
    
    int result = app.Exec();
    
    // 卸载GDI+
    DrawGrid::Inst()->UninitGdiPlus();
    AppUtil::SaveLog("[BitGrid] GDI+ shutdown");
    
    return result;
}
