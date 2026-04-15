#pragma once

#include <Windows.h>

namespace ScreenSelectOverlay
{
    // 显示一次性框选覆盖层，返回是否成功选中区域（ESC/右键取消返回 false）
    bool SelectRect(RECT& outRcScreen);
}

