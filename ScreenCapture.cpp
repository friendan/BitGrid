#include "ScreenCapture.hpp"
#include "AppUtil.hpp"

#include <gdiplus.h>
#include <string>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0;
    UINT size = 0;
    if (GetImageEncodersSize(&num, &size) != Ok || size == 0) return -1;

    auto pInfo = (ImageCodecInfo*)malloc(size);
    if (!pInfo) return -1;
    if (GetImageEncoders(num, size, pInfo) != Ok) {
        free(pInfo);
        return -1;
    }

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pInfo[j].MimeType, format) == 0) {
            *pClsid = pInfo[j].Clsid;
            free(pInfo);
            return (int)j;
        }
    }
    free(pInfo);
    return -1;
}

// 屏幕截图 需要支持多显示器
bool ScreenCapture::CaptureRectToPng(const RECT& rcScreen, const std::wstring& outPngPath, std::wstring* errMsg)
{
    int w = rcScreen.right - rcScreen.left;
    int h = rcScreen.bottom - rcScreen.top;
    if (w <= 0 || h <= 0) {
        if (errMsg) *errMsg = L"invalid rect";
        return false;
    }

    // 使用 BitBlt 从桌面截图（DWM 已合成所有窗口，包括分层窗口）
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, w, h);
    HGDIOBJ hOld = SelectObject(hdcMem, hBitmap);
    
    // 注意：不使用 CAPTUREBLT（分层窗口会导致失真）
    // DWM 已经合成所有窗口到桌面，直接 BitBlt 即可
    BOOL result = BitBlt(hdcMem, 0, 0, w, h, hdcScreen, rcScreen.left, rcScreen.top, SRCCOPY);
    
    SelectObject(hdcMem, hOld);
    
    // 转换为 GDI+ Bitmap 并保存为 PNG
    Bitmap bmp(hBitmap, nullptr);
    
    // 获取 PNG 编码器
    CLSID clsid{};
    if (GetEncoderClsid(L"image/png", &clsid) < 0) {
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdcScreen);
        if (errMsg) *errMsg = L"PNG encoder not found";
        return false;
    }
    
    Status st = bmp.Save(outPngPath.c_str(), &clsid, nullptr);
    
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    
    if (st != Ok) {
        if (errMsg) *errMsg = L"GDI+ Save failed";
        return false;
    }
    return true;
}
