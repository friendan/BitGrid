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

// ARGB 颜色宏：A=Alpha, R=Red, G=Green, B=Blue
// 注意：DIB 使用 BGRA 格式（小端序），内存中存储为 BB GG RR AA
#define ARGB(a, r, g, b) ((uint32_t)(((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))


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

    // 只有在有数据时才计算 mPageSize，避免空数据时产生错误值
    if (!mHexString.empty()) {
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
    } else {
        // 没有数据时重置分页状态
        mPageSize = 0;
        mTotalPage = 0;
        mCurPage = 1;
    }
}

// 为分层窗口初始化绘制参数
void DrawGrid::DrawInitForDIB(int width, int height){
    mWidth = width;
    mHeight = height;

    static int lineOffset = AppConst::BORDER_LINE_OFFSET;
    static int lineCount = AppConst::BORDER_LINE_COUNT;
    // 像素数据区域宽度 = 总宽 - 左右边框偏移 - 左右边框线条数 + 1（因为边框线条只在内侧）
    mDrawWidth  = mWidth  - lineOffset*2 - lineCount*2 + 1;
    mDrawHeight = mHeight - lineOffset*2 - lineCount*2 + 1;

    // 只有在有数据时才计算 mPageSize，避免空数据时产生错误值
    if (!mHexString.empty()) {
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
    } else {
        // 没有数据时重置分页状态
        mPageSize = 0;
        mTotalPage = 0;
        mCurPage = 1;
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

void DrawGrid::DrawPixGridToOverlay(HWND hwndOverlay, HBITMAP hBitmap, uint32_t* pPixels, int width, int height){
    if (!hwndOverlay || !hBitmap || !pPixels || width <= 0 || height <= 0) {
        return;
    }

    // 初始化绘制参数（计算 mDrawWidth, mDrawHeight, mPageSize 等）
    DrawInitForDIB(width, height);

    // 先绘制边框
    DrawBorderToDIB(pPixels, width, height);
    
    // 再绘制十六进制数据
    DrawHexStringToDIB(pPixels, width, height);

    // 使用 UpdateLayeredWindow 更新分层窗口
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    SelectObject(hdcMem, hBitmap);

    POINT ptDst = {0, 0};
    SIZE size = {(LONG)width, (LONG)height};
    POINT ptSrc = {0, 0};
    
    BLENDFUNCTION blend = {0};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;  // 使用整体透明度（0-255）
    blend.AlphaFormat = 0;            // 使用 SourceConstantAlpha，不使用像素级 Alpha

    RECT rcWindow;
    GetWindowRect(hwndOverlay, &rcWindow);
    ptDst.x = rcWindow.left;
    ptDst.y = rcWindow.top;

    UpdateLayeredWindow(
        hwndOverlay,
        hdcScreen,
        &ptDst,
        &size,
        hdcMem,
        &ptSrc,
        0,
        &blend,
        ULW_ALPHA
    );

    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

//=============================================================================
// 辅助函数：直接在 DIB 像素数组上绘制边框（用于分层窗口）
//=============================================================================
void DrawGrid::DrawBorderToDIB(uint32_t* pPixels, int width, int height) {
    if (!pPixels || width <= 0 || height <= 0) {
        return;
    }

    static COLORREF cr = AppConst::BORDER_COLOR;
    static COLORREF bkCr = AppConst::BACKGROUND_COLOR;
    
    // 将 COLORREF 转换为 ARGB 格式（注意：DIB 使用 BGRA 小端序）
    uint32_t borderColor = ARGB(255, GetRValue(cr), GetGValue(cr), GetBValue(cr));
    uint32_t backColor = ARGB(255, GetRValue(bkCr), GetGValue(bkCr), GetBValue(bkCr));
    
    static int lineOffset = AppConst::BORDER_LINE_OFFSET;
    static int lineCount = AppConst::BORDER_LINE_COUNT;
    
    // 填充背景色
    for (int i = 0; i < width * height; i++) {
        pPixels[i] = backColor;
    }
    
    float xStart = lineOffset;
    float yStart = lineOffset;
    float xMax = width - lineOffset;
    float yMax = height - lineOffset;

    // 绘制边框线条（注意：边框只在内侧区域绘制，不覆盖整个宽度/高度）
    for (int cnt = 0; cnt < lineCount; ++cnt) {
        // 顶部画N条直线（从 xStart 到 xMax-1）
        for (int x = (int)xStart; x < (int)xMax; x++) {
            pPixels[(int)(yStart + cnt) * width + x] = borderColor;
        }
        // 底部画N条直线
        for (int x = (int)xStart; x < (int)xMax; x++) {
            pPixels[(int)(yMax - cnt) * width + x] = borderColor;
        }
        // 左侧画N条直线（从 yStart 到 yMax-1）
        for (int y = (int)yStart; y < (int)yMax; y++) {
            pPixels[y * width + (int)(xStart + cnt)] = borderColor;
        }
        // 右侧画N条直线
        for (int y = (int)yStart; y < (int)yMax; y++) {
            pPixels[y * width + (int)(xMax - cnt)] = borderColor;
        }
    }
}

//=============================================================================
// 辅助函数：直接在 DIB 像素数组上绘制十六进制数据（用于分层窗口）
//=============================================================================
void DrawGrid::DrawHexStringToDIB(uint32_t* pPixels, int width, int height) {
    if (!pPixels || width <= 0 || height <= 0) {
        AppUtil::SaveLog("[DrawHexStringToDIB] ERROR: Invalid parameters - pPixels=", pPixels ? "valid" : "null", 
                         " width=", std::to_string(width), " height=", std::to_string(height));
        return;
    }
    
    // 参数有效性检查
    if (mPageSize < 1 || mHexString.empty()) {
        AppUtil::SaveLog("[DrawHexStringToDIB] ERROR: Invalid state - mPageSize=", std::to_string(mPageSize), 
                         " mHexString.size()=", std::to_string(mHexString.size()));
        return;
    }

    // 获取当前页的十六进制字符串
    std::string_view hexStringView = AppUtil::GetSubStrViewByPage(mHexString, mPageSize, mCurPage);
    if (hexStringView.empty()) {
        AppUtil::SaveLog("[DrawHexStringToDIB] WARNING: hexStringView is empty - mPageSize=", std::to_string(mPageSize),
                         " mCurPage=", std::to_string(mCurPage), " mTotalPage=", std::to_string(mTotalPage));
        return;
    }

    const static int& lineOffset = AppConst::BORDER_LINE_OFFSET;
    const static int& lineCount = AppConst::BORDER_LINE_COUNT;
    
    // DIB 使用 BGRA 格式，需要转换 BitColor
    // BitColor[0] = 0xFF000000 (黑色 ARGB) → 0xFF000000 (BGRA: B=0, G=0, R=0, A=255)
    // BitColor[1] = 0xFFFFFFFF (白色 ARGB) → 0xFFFFFFFF (BGRA: B=255, G=255, R=255, A=255)
    // 对于纯黑和纯白，ARGB 和 BGRA 是一样的
    static uint32_t BitColorBGRA[2] = {
        ARGB(255, 0, 0, 0),     // 黑色
        ARGB(255, 255, 255, 255) // 白色
    };

    int xStart = lineOffset + lineCount;
    int yStart = lineOffset + lineCount;
    // 使用与边框绘制一致的 xMax 计算方式，减去 lineCount 避免覆盖边框
    int xMax = width - lineOffset - lineCount + 1;   // 右边界（不含）
    int yMax = height - lineOffset - lineCount + 1; // 下边界（不含）

    // 记录关键参数用于调试
    int drawWidth = xMax - xStart;
    int drawHeight = yMax - yStart;
    int totalPixels = drawWidth * drawHeight;
    int expectedHexChars = (totalPixels + 3) / 4;  // 向上取整
    
    AppUtil::SaveLog("[DrawHexStringToDIB] === Drawing Info ===");
    AppUtil::SaveLog("  Canvas: width=", std::to_string(width), " height=", std::to_string(height));
    AppUtil::SaveLog("  DrawArea: xStart=", std::to_string(xStart), " yStart=", std::to_string(yStart),
                     " xMax=", std::to_string(xMax), " yMax=", std::to_string(yMax),
                     " drawWidth=", std::to_string(drawWidth), " drawHeight=", std::to_string(drawHeight));
    AppUtil::SaveLog("  Pixels: totalPixels=", std::to_string(totalPixels), " expectedHexChars=", std::to_string(expectedHexChars));
    AppUtil::SaveLog("  Data: mHexString.size()=", std::to_string(mHexString.size()),
                     " mPageSize=", std::to_string(mPageSize),
                     " mCurPage=", std::to_string(mCurPage),
                     " mTotalPage=", std::to_string(mTotalPage));
    AppUtil::SaveLog("  Current: hexStringView.size()=", std::to_string(hexStringView.size()),
                     " actualPixels=", std::to_string(hexStringView.size() * 4));
    AppUtil::SaveLog("  Match: ", (hexStringView.size() == (size_t)mPageSize) ? "OK" : "MISMATCH!");

    size_t x = xStart;
    size_t y = yStart;
    uint8_t bits[4] = {0};
    size_t pixelsDrawn = 0;
    
    for (char hexChar : hexStringView) {
        AppUtil::HexCharToBits(hexChar, bits);
        
        // 确保当前字符的 4 个 bits 都被处理
        for (int i = 0; i < 4; i++) {
            // 每次写入前检查是否需要换行
            if ((int)x >= xMax) {
                x = xStart;
                y += 1;
            }
            
            // 检查是否超出下边界（理论上不应该触发，防御性编程）
            if ((int)y >= yMax) {
                AppUtil::SaveLog("[DrawHexStringToDIB] ERROR: Drawing area overflow! y=", std::to_string(y), 
                                 " yMax=", std::to_string(yMax), " pixelsDrawn=", std::to_string(pixelsDrawn),
                                 " remainingChars=", std::to_string(hexStringView.size() - (&hexChar - hexStringView.data())));
                return;
            }
            
            // 写入像素
            pPixels[y * width + x++] = BitColorBGRA[bits[i]];
            pixelsDrawn++;
        }
    }
    
    AppUtil::SaveLog("[DrawHexStringToDIB] Completed: pixelsDrawn=", std::to_string(pixelsDrawn),
                     " finalX=", std::to_string(x), " finalY=", std::to_string(y));
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

//=============================================================================
// 辅助函数：检查指定列是否为边框列（连续BORDER_LINE_COUNT条边框线）
//=============================================================================
static bool IsBorderColumn(Gdiplus::Bitmap* bitmap, int x, int height)
{
    int borderCount = 0;
    for (int y = 0; y < height; y++) {
        Gdiplus::Color color;
        bitmap->GetPixel(x, y, &color);
        COLORREF rgbColor = DrawGrid::ColorToRGB(color);
        if (AppUtil::IsRgbColor(rgbColor, AppConst::BORDER_COLOR)) {
            borderCount++;
        }
    }
    bool result = borderCount > height * 0.5;  // 该列大部分是边框色
    return result;
}

//=============================================================================
// 辅助函数：检查指定行是否为边框行（连续BORDER_LINE_COUNT条边框线）
//=============================================================================
static bool IsBorderRow(Gdiplus::Bitmap* bitmap, int y, int width)
{
    int borderCount = 0;
    for (int x = 0; x < width; x++) {
        Gdiplus::Color color;
        bitmap->GetPixel(x, y, &color);
        COLORREF rgbColor = DrawGrid::ColorToRGB(color);
        if (AppUtil::IsRgbColor(rgbColor, AppConst::BORDER_COLOR)) {
            borderCount++;
        }
    }
    return borderCount > width * 0.5;  // 该行大部分是边框色
}

//=============================================================================
// 辅助函数：在图片中查找边框位置
// 返回：true=找到边框, false=未找到
// 要求：找到连续的BORDER_LINE_COUNT条边框线
//=============================================================================
static bool FindBorder(Gdiplus::Bitmap* bitmap, int& left, int& top, int& right, int& bottom)
{
    int width = bitmap->GetWidth();
    int height = bitmap->GetHeight();
    const int lineCount = AppConst::BORDER_LINE_COUNT;
    
    AppUtil::SaveLog("[FindBorder] Start, image size: ", std::to_string(width), " x ", std::to_string(height));
    AppUtil::SaveLog("[FindBorder] BORDER_LINE_COUNT: ", std::to_string(lineCount));
    AppUtil::SaveLog("[FindBorder] BORDER_COLOR: ", std::to_string(AppConst::BORDER_COLOR));
    AppUtil::SaveLog("[FindBorder] COLOR_THRESHOLD: ", std::to_string(AppConst::COLOR_THRESHOLD));
    
    // 从左边缘查找连续的BORDER_LINE_COUNT条垂直边框线
    left = -1;
    AppUtil::SaveLog("[FindBorder] Searching left border...");
    for (int x = 0; x <= width - lineCount && left < 0; x++) {
        bool found = true;
        for (int i = 0; i < lineCount; i++) {
            if (!IsBorderColumn(bitmap, x + i, height)) {
                found = false;
                break;
            }
        }
        if (found) {
            left = x;
            AppUtil::SaveLog("[FindBorder] Left border found at x=", std::to_string(x));
            break;
        }
    }
    if (left < 0) {
        AppUtil::SaveLog("[FindBorder] Left border not found");
    }
    
    // 从右边缘查找连续的BORDER_LINE_COUNT条垂直边框线
    right = -1;
    for (int x = width - 1; x >= lineCount - 1; x--) {
        bool found = true;
        for (int i = 0; i < lineCount; i++) {
            if (!IsBorderColumn(bitmap, x - i, height)) {
                found = false;
                break;
            }
        }
        if (found) {
            right = x;
            break;
        }
    }
    
    // 从上边缘查找连续的BORDER_LINE_COUNT条水平边框线
    top = -1;
    for (int y = 0; y <= height - lineCount; y++) {
        bool found = true;
        for (int i = 0; i < lineCount; i++) {
            if (!IsBorderRow(bitmap, y + i, width)) {
                found = false;
                break;
            }
        }
        if (found) {
            top = y;
            break;
        }
    }
    
    // 从下边缘查找连续的BORDER_LINE_COUNT条水平边框线
    bottom = -1;
    for (int y = height - 1; y >= lineCount - 1; y--) {
        bool found = true;
        for (int i = 0; i < lineCount; i++) {
            if (!IsBorderRow(bitmap, y - i, width)) {
                found = false;
                break;
            }
        }
        if (found) {
            bottom = y;
            break;
        }
    }
    
    // 验证是否找到所有边框
    AppUtil::SaveLog("[FindBorder] left=", std::to_string(left)
        , " right=", std::to_string(right)
        , " top=", std::to_string(top)
        , " bottom=", std::to_string(bottom)
    );
    
    if (left < 0 || right < 0 || top < 0 || bottom < 0) {
        AppUtil::SaveLog("[FindBorder] Failed: some border not found");
        return false;
    }
    
    // 验证能组成有效矩形：left < right 且 top < bottom
    if (left >= right || top >= bottom) {
        AppUtil::SaveLog("[FindBorder] Failed: invalid rectangle");
        return false;
    }
    
    // 验证边框宽度一致（左右边框间距等于绘制区域宽度 + 2*lineOffset + 2*lineCount）
    int borderWidth = right - left + 1;
    int expectedMinWidth = lineCount * 2 + 2;  // 至少要有左右边框+偏移
    if (borderWidth < expectedMinWidth) {
        AppUtil::SaveLog("[FindBorder] Failed: borderWidth too small: ", std::to_string(borderWidth));
        return false;
    }
    
    // 验证边框高度一致
    int borderHeight = bottom - top + 1;
    int expectedMinHeight = lineCount * 2 + 2;  // 至少要有上下边框+偏移
    if (borderHeight < expectedMinHeight) {
        AppUtil::SaveLog("[FindBorder] Failed: borderHeight too small: ", std::to_string(borderHeight));
        return false;
    }
    
    AppUtil::SaveLog("[FindBorder] Success");
    return true;
}

COLORREF DrawGrid::ColorToRGB(const Gdiplus::Color& color){
    return RGB(color.GetR(), color.GetG(), color.GetB());
}

//=============================================================================
// 从单张图片还原十六进制字符串
//=============================================================================
std::string DrawGrid::RestoreFromImage(const std::wstring& imagePath, 
                                        std::string* outFileName,
                                        std::string* outFileContentHex,
                                        bool isFirstPage)
{
    std::string result;
    
    // 清空输出参数
    if (outFileName) outFileName->clear();
    if (outFileContentHex) outFileContentHex->clear();
    
    AppUtil::SaveLog("[RestoreFromImage] Start");
    AppUtil::SaveLog("[RestoreFromImage] Image path: ", AppUtil::WStrToStr(imagePath));
    AppUtil::SaveLog("[RestoreFromImage] isFirstPage: ", isFirstPage ? "true" : "false");
    
    // 加载图片
    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(imagePath.c_str());
    if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok) {
        AppUtil::SaveLog("[RestoreFromImage] Failed to load image");
        if (bitmap) {
            AppUtil::SaveLog("[RestoreFromImage] Bitmap status: ", std::to_string(bitmap->GetLastStatus()));
            delete bitmap;
        }
        return result;
    }
    
    int width = bitmap->GetWidth();
    int height = bitmap->GetHeight();
    AppUtil::SaveLog("[RestoreFromImage] Image size: ", std::to_string(width), " x ", std::to_string(height));
    
    // 查找边框位置
    int left, top, right, bottom;
    AppUtil::SaveLog("[RestoreFromImage] Finding border...");
    if (!FindBorder(bitmap, left, top, right, bottom)) {
        AppUtil::SaveLog("[RestoreFromImage] FindBorder failed");
        delete bitmap;
        return result;
    }
    
    AppUtil::SaveLog("[RestoreFromImage] Border found: left=", std::to_string(left), 
                     " top=", std::to_string(top), 
                     " right=", std::to_string(right), 
                     " bottom=", std::to_string(bottom));
    
    const static int& lineOffset = AppConst::BORDER_LINE_OFFSET;
    const static int& lineCount = AppConst::BORDER_LINE_COUNT;
    
    AppUtil::SaveLog("[RestoreFromImage] lineOffset=", std::to_string(lineOffset), " lineCount=", std::to_string(lineCount));
    
    // 计算真实绘制区域（去掉边框）
    // 边框有lineCount条线，数据从边框内侧开始
    int xStart = left + lineCount;
    int yStart = top + lineCount;
    int xEnd = right - lineCount + 1;
    int yEnd = bottom - lineCount + 1;
    
    AppUtil::SaveLog("[RestoreFromImage] Draw area: xStart=", std::to_string(xStart), 
                     " yStart=", std::to_string(yStart), 
                     " xEnd=", std::to_string(xEnd), 
                     " yEnd=", std::to_string(yEnd));
    
    int drawWidth = xEnd - xStart;
    int drawHeight = yEnd - yStart;
    
    AppUtil::SaveLog("[RestoreFromImage] Draw size: ", std::to_string(drawWidth), " x ", std::to_string(drawHeight));
    
    if (drawWidth <= 0 || drawHeight <= 0) {
        AppUtil::SaveLog("[RestoreFromImage] Invalid draw size");
        delete bitmap;
        return result;
    }
    
    // 读取像素并还原为十六进制字符串
    uint8_t bits[4] = {0};
    int bitIndex = 0;
    int totalPixels = 0;
    int errorRow = 0;
    AppUtil::SaveLog("[RestoreFromImage] Starting pixel processing...");

    for (int y = yStart; y < yEnd; y++) {
        if(errorRow >= 2){
            break; // 连续2行没有 后面肯定是没有了
        }
        for (int x = xStart; x < xEnd; x++) {
            Gdiplus::Color color;
            bitmap->GetPixel(x, y, &color);

            COLORREF rgbColor = ColorToRGB(color);
            uint8_t bit = AppUtil::GetRgbColorBit(rgbColor);

            // AppUtil::SaveLog("[RestoreFromImage] x y ", x, " ", y);

            if (bit == 255) {
               errorRow += 1;
               break;  // 无效颜色（背景色?有时候一行画不满）
            }
            
            totalPixels++;
            bits[bitIndex++] = bit;
            if (bitIndex >= 4) {
                // AppUtil::SaveLog(" x y ", x-3, " ", y
                //     , " bits: "
                //     , bits[0], " "
                //     , bits[1], " "
                //     , bits[2], " "
                //     , bits[3]
                // );
                result += AppUtil::BitsToHexChar(bits);
                bitIndex = 0;
            }
        }
    }
    
    AppUtil::SaveLog("[RestoreFromImage] Pixel processing done");
    AppUtil::SaveLog("[RestoreFromImage] Total pixels: ", std::to_string(totalPixels));
    AppUtil::SaveLog("[RestoreFromImage] Result length: ", std::to_string(result.length()));
    AppUtil::SaveLog("[RestoreFromImage] Remaining bits: ", std::to_string(bitIndex));
    AppUtil::SaveLog("[RestoreFromImage] Result: ", result);
        
    delete bitmap;
    AppUtil::SaveLog("[RestoreFromImage] End");

    // 只有第一页包含文件名信息（前512个十六进制字符 = 256字节）
    std::string fileContentHex = result;
    if (isFirstPage && result.size() >= 512) {
        std::string fileNameHex = result.substr(0, 512);
        std::string fileName = AppUtil::HexStrToStr(fileNameHex);
        size_t realNameEnd = fileName.find_last_not_of('\0'); // 找到最后一个不是null的字符
        if (realNameEnd != std::string::npos) {
            fileName = fileName.substr(0, realNameEnd + 1);
        }
        fileContentHex = result.substr(512); // 去掉文件名部分
        
        AppUtil::SaveLog("[RestoreFromImage] File name: ", fileName);
        AppUtil::SaveLog("[RestoreFromImage] File content hex length: ", std::to_string(fileContentHex.length()));
        
        // 通过输出参数返回解析结果
        if (outFileName) {
            *outFileName = fileName;
        }
    } else if (!isFirstPage) {
        // 非第一页：所有数据都是文件内容
        AppUtil::SaveLog("[RestoreFromImage] Non-first page, all data is file content");
        AppUtil::SaveLog("[RestoreFromImage] File content hex length: ", std::to_string(fileContentHex.length()));
    }
    
    if (outFileContentHex) {
        *outFileContentHex = fileContentHex;
    }
    
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
std::string DrawGrid::RestoreFromFolder(const std::wstring& folderPath,
                                         std::string* outFileName,
                                         std::string* outFileContentHex)
{
    std::string result;
    
    // 清空输出参数
    if (outFileName) outFileName->clear();
    if (outFileContentHex) outFileContentHex->clear();
    
    // 获取所有图片文件并按创建时间排序
    std::vector<FileInfo> files = GetImageFilesSorted(folderPath);
    
    AppUtil::SaveLog("[RestoreFromFolder] Found ", std::to_string(files.size()), " image files");
    
    // 依次处理每个文件
    std::string allFileContentHex;
    for (size_t i = 0; i < files.size(); i++) {
        AppUtil::SaveLog("[RestoreFromFolder] Processing file ", std::to_string(i + 1), "/", std::to_string(files.size()), ": ", AppUtil::WStrToStr(files[i].path));
        
        std::string pageFileName;
        std::string pageFileContentHex;
        bool isFirst = (i == 0);
        std::string pageData = RestoreFromImage(files[i].path, &pageFileName, &pageFileContentHex, isFirst);
        
        result += pageData;
        allFileContentHex += pageFileContentHex;
        
        // 第一页包含文件名信息
        if (isFirst && outFileName && !pageFileName.empty()) {
            *outFileName = pageFileName;
        }
    }
    
    // 返回合并后的文件内容
    if (outFileContentHex) {
        *outFileContentHex = allFileContentHex;
    }
    
    AppUtil::SaveLog("[RestoreFromFolder] Total result length: ", std::to_string(result.length()));
    if (outFileName && !outFileName->empty()) {
        AppUtil::SaveLog("[RestoreFromFolder] File name: ", *outFileName);
    }
    if (outFileContentHex) {
        AppUtil::SaveLog("[RestoreFromFolder] File content hex length: ", std::to_string(outFileContentHex->length()));
    }
    
    return result;
}

