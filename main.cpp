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
#include "toml.hpp"
#include "picosha2.h"
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
    
    /// 配置文件路径
    std::wstring ConfigPath() {
        return PathUtil::GetExeDir() + L"\\BitGrid.toml";
    }
    
    /// 保存区域配置到文件
    void SaveConfig() {
        auto path = ConfigPath();
        auto pathA = AppUtil::WStrToStr(path);
        try {
            toml::table tbl;
            tbl.emplace("select_left", selectedRectScreen.left);
            tbl.emplace("select_top", selectedRectScreen.top);
            tbl.emplace("select_right", selectedRectScreen.right);
            tbl.emplace("select_bottom", selectedRectScreen.bottom);
            
            // 保存总页数输入框的值
            int totalPage = GetTotalPage();
            tbl.emplace("total_page", totalPage);
            
            std::ofstream ofs(pathA);
            if (ofs) {
                ofs << tbl << std::endl;
                AppUtil::SaveLog("[Config] Saved: ", pathA);
            }
        } catch (std::exception& e) {
            AppUtil::SaveLog("[Config] Save error: ", e.what());
        }
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
    
    /// 设置总页数输入框的值
    void SetTotalPage(int page) {
        auto* txt = (TextBox*)this->FindControl("txtTotalPage");
        if (txt) {
            txt->SetText(std::to_wstring(page).c_str());
        }
    }
    
    /// 从文件读取配置
    void LoadConfig() {
        auto path = ConfigPath();
        auto pathA = AppUtil::WStrToStr(path);
        
        std::ifstream ifs(pathA);
        if (!ifs.is_open()) {
            return;  // 配置文件不存在，不读取
        }
        
        try {
            auto tbl = toml::parse(ifs);
            
            auto left   = tbl["select_left"].value<int>();
            auto top    = tbl["select_top"].value<int>();
            auto right  = tbl["select_right"].value<int>();
            auto bottom = tbl["select_bottom"].value<int>();
            
            if (left && top && right && bottom) {
                selectedRectScreen = { *left, *top, *right, *bottom };
                hasSelection = true;
                
                AddLog(L"[INFO] 已从配置文件还原区域: (" +
                    std::to_wstring(*left) + L"," + std::to_wstring(*top) + L")-(" +
                    std::to_wstring(*right) + L"," + std::to_wstring(*bottom) + L")");
                ScreenSelectOverlay::ShowBorder(selectedRectScreen);
            }
            
            // 还原总页数
            auto totalPage = tbl["total_page"].value<int>();
            if (totalPage) {
                SetTotalPage(*totalPage);
                AddLog(L"[INFO] 已还原总页数: " + std::to_wstring(*totalPage));
            }
        } catch (std::exception& e) {
            AppUtil::SaveLog("[Config] Load error: ", e.what());
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
        
        // 读取配置文件
        LoadConfig();
        
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
    
    /// 在后台线程中更新状态栏右侧文本
    void PostStatusRight(const std::wstring& msg) {
        PostMessage(this->Hwnd(), WM_USER + 102, 0,
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
    }
    
    /// 计算文件的 SHA256
    std::string CalcFileHash(const std::wstring& path) {
        std::ifstream ifs(AppUtil::WStrToStr(path), std::ios::binary);
        if (!ifs) return "";
        return picosha2::hash256_hex_string(
            std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>());
    }
    
    /// 截图并校验，成功返回 true，失败则内部自动终止流程
    bool CaptureAndVerify(const std::wstring& path, const std::wstring& dir, int page, int totalPage) {
        std::wstring err;
        if (!ScreenCapture::CaptureRectToPng(selectedRectScreen, path, &err)) {
            PostLog(L"[ERROR] 截图失败: " + err);
            return false;
        }
        PostLog(L"[INFO] 已截图(" + std::to_wstring(page) + L"/" + std::to_wstring(totalPage) + L"): " + path);
        
        std::string pageFileName, pageContentHex;
        std::string pageData = DrawGrid::RestoreFromImage(path,
            &pageFileName, &pageContentHex, (page == 1));
        if (pageData.empty()) {
            PostLog(L"[ERROR] CRC32 校验失败，截图可能异常");
            return false;
        }
        
        // 不是第1张图片，要和上一张比较hash，防止重复截图
        if (page > 1) {
            std::wstring prevPath = dir + L"\\" + std::to_wstring(page - 1) + L".png";
            std::string curHash = CalcFileHash(path);
            std::string prevHash = CalcFileHash(prevPath);
            if (!prevHash.empty() && curHash == prevHash) {
                PostLog(L"[ERROR] 截图与上一张相同，翻页未成功");
                return false;
            }
        }
        
        size_t ls = path.find_last_of(L"\\");
        std::wstring sn = (ls != std::wstring::npos) ? path.substr(ls + 1) : path;
        PostStatusRight(sn);
        
        uint32_t crc32 = AppUtil::Crc32File(path);
        std::string sha256 = CalcFileHash(path);
        PostLog(L"[INFO] " + std::to_wstring(page) + L".png: CRC32=0x" + 
            AppUtil::StrToWStr(AppUtil::UInt32ToHexStr(crc32)) + L" SHA256=" + AppUtil::StrToWStr(sha256));
        
        return true;
    }
    
    /// 获取目录中已有图片数量，从 N+1 开始
    int GetStartPageFromDir(const std::wstring& dir) {
        int startPage = 1;
        std::wstring searchPath = dir + L"\\*.png";
        WIN32_FIND_DATAW ffd;
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &ffd);
        if (hFind != INVALID_HANDLE_VALUE) {
            int maxNum = 0;
            do {
                std::wstring name = ffd.cFileName;
                if (name.size() > 4 && name.substr(name.size() - 4) == L".png") {
                    std::wstring numStr = name.substr(0, name.size() - 4);
                    try {
                        int num = std::stoi(numStr);
                        if (num > maxNum) maxNum = num;
                    } catch (...) {}
                }
            } while (FindNextFileW(hFind, &ffd) != 0);
            FindClose(hFind);
            startPage = maxNum + 1;
            PostLog(L"[INFO] 目录已有 " + std::to_wstring(maxNum) + L" 张截图，从第 " + std::to_wstring(startPage) + L" 张开始");
        }
        return startPage;
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
    }
    
    /// 模拟翻页：来回移动鼠标防止空闲，再按空格翻页
    void SimulatePageTurn(int centerX, int centerY) {
        for (int i = -20; i <= 20; i += 5) {
            SetCursorPos(centerX + i, centerY);
            Sleep(3);
        }
        for (int i = 20; i >= -20; i -= 5) {
            SetCursorPos(centerX + i, centerY);
            Sleep(3);
        }
        SimulateMouseClick();
        SimulateKeyPress(VK_SPACE);
    }

    /// 自动操作：循环截图 + 翻页 + 触发识别
    void RunAutoAction() {
        int totalPage = GetTotalPage();
        PostLog(L"[INFO] 总页数: " + std::to_wstring(totalPage));
        
        // 创建截图目录（不清空，支持续传）
        std::wstring dir = PathUtil::GetTodayFolderPath();
        PathUtil::EnsureDirExists(dir);
        
        int centerX = (selectedRectScreen.left + selectedRectScreen.right) / 2;
        int centerY = (selectedRectScreen.top + selectedRectScreen.bottom) / 2;
        
        int startPage = GetStartPageFromDir(dir);
        int page = startPage;
        int errorCount = 0;
        const int maxErrors = 50;
        
        while (page <= totalPage) {
            // 检查是否被中断
            if (!isAutoActionRunning.load()) {
                PostLog(L"[INFO] 用户中断自动操作");
                return;
            }
            
            // 截图
            std::wstring pngPath = dir + L"\\" + std::to_wstring(page) + L".png";
            if (!CaptureAndVerify(pngPath, dir, page, totalPage)) {
                errorCount++;
                PostLog(L"[INFO] CRC校验失败(" + std::to_wstring(errorCount) + L"/" + std::to_wstring(maxErrors) + L"), 等待100ms重试");
                if (errorCount >= maxErrors) {
                    PostLog(L"[ERROR] CRC校验连续失败" + std::to_wstring(maxErrors) + L"次，流程终止");
                    FinishAutoAction(false);
                    return;
                }
                Sleep(100);
                continue;
            }
            
            // 校验通过，重置错误计数
            errorCount = 0;
            
            if (page >= totalPage) {
                PostLog(L"[INFO] 所有页面截图完成");
                break;
            }
            
            // 翻页
            SimulatePageTurn(centerX, centerY);
            page++;
            Sleep(100);
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
                    SaveConfig();
                    ScreenSelectOverlay::UpdateBorder(rc);
                }
                else {
                    AddLog(L"[WARN] 已取消选择");
                    UpdateStatus(L"就绪", L"", L"");
                }
            }
            else if (sender->Name == "btnShot") {
                ScreenSelectOverlay::HideBorder();
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
                
                // CRC32 校验
                {
                    std::string pageFileName;
                    std::string pageContentHex;
                    std::string pageData = DrawGrid::RestoreFromImage(outPng,
                        &pageFileName, &pageContentHex, false);
                    if (pageData.empty()) {
                        AddLog(L"[ERROR] CRC32 校验失败，截图可能异常");
                        UpdateStatus(L"截图异常", L"", L"");
                    } else {
                        // 状态栏显示文件名
                        size_t lastSlash = outPng.find_last_of(L"\\");
                        std::wstring shortName = (lastSlash != std::wstring::npos) ? outPng.substr(lastSlash + 1) : outPng;
                        UpdateStatus(L"已截图", shortName, L"");
                    }
                }
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
                
                ScreenSelectOverlay::HideBorder();
                // 保存配置
                SaveConfig();
                
                isAutoActionRunning.store(true);
                sender->SetEnabled(false);
                
                // 启动后台线程执行自动操作
                std::thread([this]() {
                    RunAutoAction();
                }).detach();
            }
            else if (sender->Name == "btnRecognize") {
                // 保存配置
                SaveConfig();
                
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
                ScreenSelectOverlay::HideBorder();
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
            
            // 强制刷新状态栏和窗口
            InvalidateRect(this->Hwnd(), nullptr, TRUE);
            
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
        else if (msg == WM_USER + 102) {
            // 状态栏右侧更新消息
            auto* pMsg = reinterpret_cast<std::wstring*>(lParam);
            if (pMsg && statusRight) {
                statusRight->SetText(*pMsg);
                InvalidateRect(this->Hwnd(), nullptr, TRUE);
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
