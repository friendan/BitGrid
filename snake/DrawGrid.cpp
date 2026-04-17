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
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // mHexString = "";
    // for(int cnt = 1; cnt <= 18; cnt++){
    //     mHexString += "0123456789ABCDEF";
    // }
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

// 缓存结构体，用于管理 GDI 资源
struct GdiCache {
    HBITMAP hBitmap = nullptr;
    HDC hdcMem = nullptr;
    uint32_t* pixels = nullptr;
    size_t width = 0;
    size_t height = 0;

    ~GdiCache() {
        if (hdcMem) {
            DeleteDC(hdcMem);
        }
        if (hBitmap) {
            DeleteObject(hBitmap);
        }
        pixels = nullptr;
    }
};

void DrawGrid::DrawHexString(HWND hwnd, HDC hdc){
    // 参数有效性检查
    if(mWidth < 1 || mHeight < 1 || mPageSize < 1 || mHexString.empty() || mHexString.size() % 2 != 0){
        // AppUtil::SaveLog("DrawHexString param error");
        // AppUtil::SaveLog("mWidth:", mWidth, " mHeight:", mHeight, " mPageSize:", mPageSize, " mCurPage:", mCurPage);
        // AppUtil::SaveLog("mHexString:", mHexString);
        return;
    }

    // 获取当前页的十六进制字符串（使用视图而非复制）
    std::string_view hexStringView = AppUtil::GetSubStrViewByPage(mHexString, mPageSize, mCurPage);
    if(hexStringView.empty()){
        // AppUtil::SaveLog("hexStringView is empty mPageSize:", mPageSize, " mCurPage:", mCurPage);
        return;
    }

    // 计算绘制区域位置
    const static int& lineOffset = AppConst::BORDER_LINE_OFFSET;
    const static int& lineCount = AppConst::BORDER_LINE_COUNT;
    const static uint32_t* BitColor = AppConst::BitColor;

    int xStart = lineOffset + lineCount;
    int yStart = lineOffset + lineCount;

     // AppUtil::SaveLog("hexStringView:", hexStringView);
    // AppUtil::SaveLog("mWidth:", mWidth, " mHeight:", mHeight);
    // AppUtil::SaveLog("mDrawWidth:", mDrawWidth, " mDrawHeight:", mDrawHeight);
    // AppUtil::SaveLog("xStart:", xStart, " yStart:", yStart);
    // AppUtil::SaveLog("xMax:", xMax, " yMax:", yMax);
    // AppUtil::SaveLog("mPageSize:", mPageSize, " mCurPage:", mCurPage);
    // AppUtil::SaveLog("bitTotal:", hexStringView.size()*4);

    // 碰到的问题：
    // 行尾剩余空间 不足 4 个像素 时
    // 你依然强行写 4 个
    // 直接越界写到下一行，覆盖数据

    // 严格按顺序往 pixels 数组里线性填充（0 → 1 → 2 → 3 → ... 一直往后写）
    // 不需要判断一行够不够 4 个像素
    // 不需要自动换行
    // 数组空间一定足够，只管顺序写满

    // 静态缓存，避免频繁创建 GDI 资源
    static GdiCache cache;

    // 检查缓存是否有效
    if (cache.width != mDrawWidth || cache.height != mDrawHeight) {
        // 缓存无效，重新创建
        cache = GdiCache(); // 调用析构函数释放旧资源

        // 创建 DIB Section
        BITMAPINFO bmi = {0};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = static_cast<LONG>(mDrawWidth);
        bmi.bmiHeader.biHeight = -static_cast<LONG>(mDrawHeight);
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32; // ARGB
        bmi.bmiHeader.biCompression = BI_RGB;

        cache.hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&cache.pixels, NULL, 0);
        if (!cache.hBitmap || !cache.pixels) {
            return; // 创建失败
        }

        cache.hdcMem = CreateCompatibleDC(hdc);
        if (!cache.hdcMem) {
            return; // 创建失败
        }

        SelectObject(cache.hdcMem, cache.hBitmap);
        cache.width = mDrawWidth;
        cache.height = mDrawHeight;
    }

    // 填充背景色
    static uint32_t bkColor = 0xFF000000 | AppConst::BACKGROUND_COLOR;
    size_t pixelCount = mDrawWidth * mDrawHeight;
    for(size_t i = 0; i < pixelCount; i++){
        cache.pixels[i] = bkColor;
    }

    size_t index = 0;
    uint8_t bits[4] = {0};
    for(char hexChar: hexStringView){
        AppUtil::HexCharToBits(hexChar, bits);
        cache.pixels[index++] = BitColor[bits[0]];
        cache.pixels[index++] = BitColor[bits[1]];
        cache.pixels[index++] = BitColor[bits[2]];
        cache.pixels[index++] = BitColor[bits[3]];

        // AppUtil::SaveLog("hexChar:", hexChar
        //     , " index: ", index
        //     , " bits: "
        //     , bits[0], " "
        //     , bits[1], " "
        //     , bits[2], " "
        //     , bits[3]
        // );
    }

    // 将绘制内容复制到窗口
    BitBlt(hdc, xStart, yStart, static_cast<int>(mDrawWidth), static_cast<int>(mDrawHeight), cache.hdcMem, 0, 0, SRCCOPY);
    // AppUtil::SaveLog("DrawHexString finish");
}


