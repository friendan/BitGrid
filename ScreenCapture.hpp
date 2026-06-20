#pragma once

#include <Windows.h>
#include <string>
#include <gdiplus.h>

namespace ScreenCapture
{
    // 将屏幕指定矩形区域保存为 PNG 文件（需要外部已 InitGdiPlus）
    bool CaptureRectToPng(const RECT& rcScreen, const std::wstring& outPngPath, std::wstring* errMsg = nullptr);
    
    // 截图到内存，返回 Gdiplus::Bitmap*（调用方负责 delete），失败返回 nullptr
    Gdiplus::Bitmap* CaptureToBitmap(const RECT& rcScreen);
    
    // 将内存 Bitmap 保存为 PNG 文件
    bool SaveBitmapToPng(Gdiplus::Bitmap* bitmap, const std::wstring& outPngPath);
}

