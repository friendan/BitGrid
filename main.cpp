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
    RECT selectedStatusBarRect{};
    bool hasSelection = false;
    bool hasStatusBarSelection = false;
    std::atomic<bool> isRecognizing{false};  // 是否正在识别
    
public:
    MainFrm(int width, int height) : Window(width, height) {
        // AppUtil::SaveLog("[BitGrid] MainFrm constructor started");
        Init();
        // AppUtil::SaveLog("[BitGrid] MainFrm constructor completed");
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
        
        UpdateStatus(L"就绪", L"", L"");
    }
    
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
            else if (sender->Name == "btnSelectStatusBar") {
                AddLog(L"[INFO] 进入选择模式：拖拽框选状态栏区域，ESC/右键取消");
                RECT rc{};
                if (ScreenSelectOverlay::SelectRect(rc)) {
                    selectedStatusBarRect = rc;
                    hasStatusBarSelection = true;
                    AddLog(L"[INFO] 已选择状态栏区域: (" + std::to_wstring(rc.left) + L"," + std::to_wstring(rc.top) + L")-(" +
                        std::to_wstring(rc.right) + L"," + std::to_wstring(rc.bottom) + L")");
                }
                else {
                    AddLog(L"[WARN] 已取消选择状态栏区域");
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
                AddLog(L"[TODO] 自动操作功能尚未实现");
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
    
    // 初始化 MNN OCR 引擎
    // 自动搜索 exe 同目录下的 MNN_dbg.dll / MNN.dll
    {
        static MnnOcr s_ocr;
        if (!s_ocr.load() || !s_ocr.init(4)) {
            AppUtil::SaveLog("[MnnOcr] OCR engine init failed (MNN.dll not found?)");
        }
    }
    
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
