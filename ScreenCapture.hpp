#pragma once

#include <Windows.h>
#include <string>

namespace ScreenCapture
{
    // 将屏幕指定矩形区域保存为 PNG 文件（需要外部已 InitGdiPlus）
    bool CaptureRectToPng(const RECT& rcScreen, const std::wstring& outPngPath, std::wstring* errMsg = nullptr);
}

