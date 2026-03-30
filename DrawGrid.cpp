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


