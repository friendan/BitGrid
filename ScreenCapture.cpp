#include "ScreenCapture.hpp"

#include <gdiplus.h>
#include <string>

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

bool ScreenCapture::CaptureRectToPng(const RECT& rcScreen, const std::wstring& outPngPath, std::wstring* errMsg)
{
    int w = rcScreen.right - rcScreen.left;
    int h = rcScreen.bottom - rcScreen.top;
    if (w <= 0 || h <= 0) {
        if (errMsg) *errMsg = L"invalid rect";
        return false;
    }

    HDC hdcScreen = GetDC(nullptr);
    if (!hdcScreen) {
        if (errMsg) *errMsg = L"GetDC(NULL) failed";
        return false;
    }

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (!hdcMem) {
        ReleaseDC(nullptr, hdcScreen);
        if (errMsg) *errMsg = L"CreateCompatibleDC failed";
        return false;
    }

    HBITMAP hbmp = CreateCompatibleBitmap(hdcScreen, w, h);
    if (!hbmp) {
        DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdcScreen);
        if (errMsg) *errMsg = L"CreateCompatibleBitmap failed";
        return false;
    }

    HGDIOBJ old = SelectObject(hdcMem, hbmp);
    BOOL ok = BitBlt(hdcMem, 0, 0, w, h, hdcScreen, rcScreen.left, rcScreen.top, SRCCOPY | CAPTUREBLT);
    SelectObject(hdcMem, old);

    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);

    if (!ok) {
        DeleteObject(hbmp);
        if (errMsg) *errMsg = L"BitBlt failed";
        return false;
    }

    Bitmap bmp(hbmp, nullptr);
    DeleteObject(hbmp);

    CLSID clsid{};
    if (GetEncoderClsid(L"image/png", &clsid) < 0) {
        if (errMsg) *errMsg = L"PNG encoder not found";
        return false;
    }

    Status st = bmp.Save(outPngPath.c_str(), &clsid, nullptr);
    if (st != Ok) {
        if (errMsg) *errMsg = L"GDI+ Save failed, status=" + std::to_wstring((int)st);
        return false;
    }
    return true;
}

