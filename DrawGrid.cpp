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
    AppUtil::SaveLog("[FindBorder] left=", std::to_string(left), " right=", std::to_string(right), 
                     " top=", std::to_string(top), " bottom=", std::to_string(bottom));
    
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
                                        std::string* outFileContentHex)
{
    std::string result;
    
    // 清空输出参数
    if (outFileName) outFileName->clear();
    if (outFileContentHex) outFileContentHex->clear();
    
    AppUtil::SaveLog("[RestoreFromImage] Start");
    AppUtil::SaveLog("[RestoreFromImage] Image path: ", AppUtil::WStrToStr(imagePath));
    
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
    AppUtil::SaveLog("[RestoreFromImage] Starting pixel processing...");

    for (int y = yStart; y < yEnd; y++) {
        for (int x = xStart; x < xEnd; x++) {
            Gdiplus::Color color;
            bitmap->GetPixel(x, y, &color);

            COLORREF rgbColor = ColorToRGB(color);
            uint8_t bit = AppUtil::GetRgbColorBit(rgbColor);

            AppUtil::SaveLog("x ", x
                , " y ", y
                , " rgbColor: ", rgbColor&&0xFFFFFFFF
            );

            if (bit == 255) {
               break;  // 无效颜色（背景色?）
            }
            
            totalPixels++;
            bits[bitIndex++] = bit;
            if (bitIndex >= 4) {
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

    // 文件名正好是256字节 = 512个十六进制字符
    if(result.size() < 512){
        return "";
    }
    std::string fileNameHex = result.substr(0, 512);
    std::string fileName = AppUtil::HexStrToStr(fileNameHex);
    size_t realNameEnd = fileName.find_last_not_of('0'); // 找到最后一个不是'0'的字符的下标
    if (realNameEnd != std::string::npos) {
        fileName = fileName.substr(0, realNameEnd + 1); // 去掉末尾的填充'0'字符（不是null字符）
    }
    std::string fileContentHex = result.substr(512);
    AppUtil::SaveLog("[RestoreFromImage] File name: ", fileName);
    AppUtil::SaveLog("[RestoreFromImage] File content hex length: ", std::to_string(fileContentHex.length()));

    // 通过输出参数返回解析结果
    if (outFileName) {
        *outFileName = fileName;
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
        std::string pageData = RestoreFromImage(files[i].path, &pageFileName, &pageFileContentHex);
        
        result += pageData;
        allFileContentHex += pageFileContentHex;
        
        // 第一页包含文件名信息
        if (i == 0 && outFileName && !pageFileName.empty()) {
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
