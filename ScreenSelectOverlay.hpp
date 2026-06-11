#pragma once

#include <Windows.h>

namespace ScreenSelectOverlay
{
    // 显示一次性框选覆盖层，返回是否成功选中区域（ESC/右键取消返回 false）
    bool SelectRect(RECT& outRcScreen);
    
    // 在选择后持续显示选择框（绿色边框覆盖层）
    // 传入之前选中的屏幕区域，调用 Show/Hide 控制显示
    void ShowBorder(const RECT& rcScreen);
    void HideBorder();
    void UpdateBorder(const RECT& rcScreen);
}

