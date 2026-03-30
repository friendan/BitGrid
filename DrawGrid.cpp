#include "AppUtil.hpp"
#include "AppConst.hpp"
#include "DrawGrid.hpp"
#include <cuchar>
#include <stdexcept>
#include <clocale>
#include <fstream>
#include <sstream>
#include <iomanip>
using namespace Gdiplus;


//=============================================================================
// 初始化 GDI+
//=============================================================================
void DrawGrid::InitGdiPlus()
{
    gdiplusStartupInput = Gdiplus::GdiplusStartupInput();
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

//=============================================================================
// 卸载 GDI+
//=============================================================================
void DrawGrid::UninitGdiPlus()
{
    if (gdiplusToken != NULL)
    {
        GdiplusShutdown(gdiplusToken);
        gdiplusToken = NULL;
    }
}

void DrawGrid::DrawInit(HWND hwnd, HDC hdc){
    RECT rcClient{};
    GetClientRect(hwnd, &rcClient);
    mWidth = rcClient.right - rcClient.left;
    mHeight = rcClient.bottom - rcClient.top - AppConst::TASKBAR_HEIGHT;

    static int lineOffset = AppConst::BORDER_LINE_OFFSET;
    static int lineCount = AppConst::BORDER_LINE_COUNT;
    mDrawWidth  = mWidth  - lineOffset*2 - lineCount*2 + 1;
    mDrawHeight = mHeight - lineOffset*2 - lineCount*2 + 1;

    mPageSize = mDrawWidth*mDrawHeight / 4;
    if(mPageSize > 0){
        mTotalPage = mHexString.size() / mPageSize;
        if(mHexString.size() % mPageSize != 0){
            mTotalPage += 1;
        }
    }
    if(mTotalPage < 1){
        mTotalPage = 1;
    }
}

void DrawGrid::DrawBorder(HWND hwnd, HDC hdc)
{
    if (!hwnd || !hdc) return;

    static COLORREF cr = AppConst::BORDER_COLOR;
    static COLORREF bkCr = AppConst::BACKGROUND_COLOR;

    Pen blackPen(Color(GetRValue(cr), GetGValue(cr), GetBValue(cr)), 1.0f); // 画笔（1像素宽）
    Graphics graphics(hdc);

    SolidBrush blackBrush(Color(GetRValue(bkCr), GetGValue(bkCr), GetBValue(bkCr)));
    graphics.FillRectangle(&blackBrush, 0, 0, mWidth, mHeight);

    static int lineOffset = AppConst::BORDER_LINE_OFFSET;
    static int lineCount = AppConst::BORDER_LINE_COUNT;
    float xStart = lineOffset;
    float yStart = lineOffset;
    float xMax = mWidth - lineOffset;
    float yMax = mHeight - lineOffset;

    for(int cnt = 0; cnt < lineCount; ++cnt){
        graphics.DrawLine(&blackPen, xStart, yStart + cnt, xMax, yStart + cnt);     // 顶部画N条直线
        graphics.DrawLine(&blackPen, xStart, yMax   - cnt, xMax, yMax   - cnt);     // 底部画N条直线
        graphics.DrawLine(&blackPen, xStart + cnt, yStart, xStart + cnt, yMax);     // 左侧画N条直线
        graphics.DrawLine(&blackPen, xMax - cnt,   yStart, xMax   - cnt, yMax);     // 右侧画N条直线
    }
}

void DrawGrid::DrawPixGrid(HWND hwnd){
    PAINTSTRUCT ps;
    HDC hdc = ::BeginPaint(hwnd, &ps);
        DrawInit(hwnd, hdc);
        DrawBorder(hwnd, hdc);
        DrawHexString(hwnd, hdc);
    ::EndPaint(hwnd, &ps);
}

void DrawGrid::SetHexString(const std::string& hexString){
    mHexString = hexString;
    mCurPage = 1;
}

void DrawGrid::NextPage(){
    mCurPage += 1;
    if(mCurPage > mTotalPage){
        mCurPage = mTotalPage;
    }
}

void DrawGrid::ChangePage(int chVal){
    if(mCurPage < 1){
        mCurPage = 1;
    }
    mCurPage += chVal;
    if(mCurPage > mTotalPage){
        mCurPage = mTotalPage;
    }
   if(mCurPage < 1){
        mCurPage = 1;
    }
}

void DrawGrid::DrawHexString(HWND hwnd, HDC hdc){
    if(mWidth < 1 || mHeight < 1 || mPageSize < 1 || mHexString.empty() || mHexString.size() % 2 != 0){
        AppUtil::SaveLog("DrawHexString param error");
        AppUtil::SaveLog("mWidth:", mWidth, " mHeight:", mHeight, " mPageSize:", mPageSize, " mCurPage:", mCurPage);
        AppUtil::SaveLog("mHexString:", mHexString);
        return;
    }

    std::string hexString = AppUtil::GetSubStrByPage(mHexString, mPageSize, mCurPage);
    if(hexString.empty()){
        AppUtil::SaveLog("hexString is empty mPageSize:", mPageSize, " mCurPage:", mCurPage);
        return;
    }

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = mDrawWidth;
    bmi.bmiHeader.biHeight = -mDrawHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32; // ARGB
    bmi.bmiHeader.biCompression = BI_RGB;

    uint32_t* pixels = nullptr;  // pixels[y * width + x] = color; // (x, y)
    HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&pixels, NULL, 0);
    HDC hdcMem = CreateCompatibleDC(hdc);
    SelectObject(hdcMem, hBitmap);

    static uint32_t bkColor = 0xFF000000 | AppConst::BACKGROUND_COLOR;
    for(size_t i = 0; i < mDrawWidth*mDrawHeight; i++){
        pixels[i] = bkColor;
    }

    const static int& lineOffset = AppConst::BORDER_LINE_OFFSET;
    const static int& lineCount = AppConst::BORDER_LINE_COUNT;
    const static uint32_t* BitColor = AppConst::BitColor;

    float xStart = lineOffset + lineCount;
    float yStart = lineOffset + lineCount;
    float xMax = mDrawWidth  - lineOffset - lineCount;
    float yMax = mDrawHeight - lineOffset - lineCount;

    // AppUtil::SaveLog("hexString:", hexString);
    AppUtil::SaveLog("mWidth:", mWidth, " mHeight:", mHeight);
    AppUtil::SaveLog("mDrawWidth:", mDrawWidth, " mDrawHeight:", mDrawHeight);
    AppUtil::SaveLog("xStart:", xStart, " yStart:", yStart);
    AppUtil::SaveLog("xMax:", xMax, " yMax:", yMax);
    AppUtil::SaveLog("mPageSize:", mPageSize, " mCurPage:", mCurPage);

    size_t x = 0;
    size_t y = 0;
    uint8_t bits[4] = {0};
    for(char hexChar: hexString){
        AppUtil::HexCharToBits(hexChar, bits);
        pixels[y * mDrawWidth + x++] = BitColor[bits[0]];
        pixels[y * mDrawWidth + x++] = BitColor[bits[1]];
        pixels[y * mDrawWidth + x++] = BitColor[bits[2]];
        pixels[y * mDrawWidth + x++] = BitColor[bits[3]];
        if(x >= mDrawWidth){
            x = 0;
            y += 1;
        }
    }

    BitBlt(hdc, xStart, yStart, mDrawWidth, mDrawHeight, hdcMem, 0, 0, SRCCOPY);
    DeleteDC(hdcMem);
    DeleteObject(hBitmap);
    AppUtil::SaveLog("DrawHexString finish");
}

//=============================================================================
// 辅助函数：将颜色转换为bit值
// 返回值：0=黑色, 1=白色, 255=无效颜色(背景色或其他)
// 阈值：距离黑色或白色小于等于50视为有效，否则无效
//=============================================================================
static uint8_t ColorToBit(uint32_t color)
{
    // 去掉Alpha通道，只比较RGB
    uint32_t rgb = color & 0x00FFFFFF;
    
    // 提取RGB分量
    int r1 = (rgb >> 16) & 0xFF;
    int g1 = (rgb >> 8) & 0xFF;
    int b1 = rgb & 0xFF;
    
    // 计算与黑色的距离
    int distBlack = abs(r1 - 0) + abs(g1 - 0) + abs(b1 - 0);
    
    // 计算与白色的距离
    int distWhite = abs(r1 - 0xFF) + abs(g1 - 0xFF) + abs(b1 - 0xFF);
    
    // 如果距离黑色在阈值内，返回0
    if (distBlack <= AppConst::COLOR_THRESHOLD) {
        return 0;
    }
    
    // 如果距离白色在阈值内，返回1
    if (distWhite <= AppConst::COLOR_THRESHOLD) {
        return 1;
    }
    
    // 其他情况视为无效颜色
    return 255;
}

//=============================================================================
// 从单张图片还原十六进制字符串
//=============================================================================
std::string DrawGrid::RestoreFromImage(const std::wstring& imagePath)
{
    std::string result;
    
    // 加载图片
    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(imagePath.c_str());
    if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok) {
        if (bitmap) delete bitmap;
        return result;
    }
    
    int width = bitmap->GetWidth();
    int height = bitmap->GetHeight();
    
    const static int& lineOffset = AppConst::BORDER_LINE_OFFSET;
    const static int& lineCount = AppConst::BORDER_LINE_COUNT;
    
    // 计算实际绘制区域
    int drawWidth = width - lineOffset * 2 - lineCount * 2 + 1;
    int drawHeight = height - lineOffset * 2 - lineCount * 2 + 1;
    
    if (drawWidth <= 0 || drawHeight <= 0) {
        delete bitmap;
        return result;
    }
    
    // 计算起始位置
    int xStart = lineOffset + lineCount;
    int yStart = lineOffset + lineCount;
    
    // 读取像素并还原为十六进制字符串
    uint8_t bits[4] = {0};
    int bitIndex = 0;
    
    for (int y = 0; y < drawHeight; y++) {
        for (int x = 0; x < drawWidth; x++) {
            Gdiplus::Color color;
            bitmap->GetPixel(xStart + x, yStart + y, &color);
            
            uint32_t colorValue = 0xFF000000 | (color.GetR() << 16) | (color.GetG() << 8) | color.GetB();
            uint8_t bit = ColorToBit(colorValue);
            
            // 跳过无效颜色（背景色）
            if (bit == 255) {
                continue;
            }
            
            bits[bitIndex++] = bit;
            
            if (bitIndex >= 4) {
                result += AppUtil::BitsToHexChar(bits);
                bitIndex = 0;
            }
        }
    }
    
    delete bitmap;
    return result;
}

//=============================================================================
// 辅助函数：获取文件夹中的所有图片文件并按创建时间排序
//=============================================================================
#include <vector>
#include <algorithm>

struct FileInfo {
    std::wstring path;
    FILETIME creationTime;
};

static std::vector<FileInfo> GetImageFilesSorted(const std::wstring& folderPath)
{
    std::vector<FileInfo> files;
    
    WIN32_FIND_DATAW findData;
    std::wstring searchPath = folderPath + L"\\*.*";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // 跳过目录和特殊文件
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                continue;
            }
            
            // 检查是否是图片文件
            std::wstring fileName = findData.cFileName;
            size_t dotPos = fileName.find_last_of(L'.');
            if (dotPos == std::wstring::npos) continue;
            
            std::wstring ext = fileName.substr(dotPos);
            // 转换为小写
            for (auto& c : ext) c = towlower(c);
            
            if (ext == L".png" || ext == L".jpg" || ext == L".jpeg" || 
                ext == L".bmp" || ext == L".gif" || ext == L".tiff") {
                FileInfo info;
                info.path = folderPath + L"\\" + fileName;
                info.creationTime = findData.ftCreationTime;
                files.push_back(info);
            }
        } while (FindNextFileW(hFind, &findData));
        
        FindClose(hFind);
    }
    
    // 按创建时间排序（从早到晚）
    std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
        return CompareFileTime(&a.creationTime, &b.creationTime) < 0;
    });
    
    return files;
}

//=============================================================================
// 从文件夹还原十六进制字符串（多页情况）
//=============================================================================
std::string DrawGrid::RestoreFromFolder(const std::wstring& folderPath)
{
    std::string result;
    
    // 获取所有图片文件并按创建时间排序
    std::vector<FileInfo> files = GetImageFilesSorted(folderPath);
    
    // 依次处理每个文件
    for (const auto& file : files) {
        std::string pageData = RestoreFromImage(file.path);
        result += pageData;
    }
    
    return result;
}
