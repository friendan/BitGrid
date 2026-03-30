#pragma once
#include <Windows.h>

namespace AppConst
{
	// 二进制值(0~15) → 16进制字符
	constexpr char BinTohex[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	constexpr int BORDER_LINE_OFFSET = 2;	// 表示：从顶部/底部/左侧/右侧 数第几条线
	constexpr int BORDER_LINE_COUNT  = 3;	// 表示：从顶部/底部/左侧/右侧 依次画N条线
	constexpr COLORREF BORDER_COLOR  = RGB(255, 0, 0);
	// constexpr COLORREF BORDER_COLOR  = RGB(0, 0, 0);
	constexpr COLORREF BACKGROUND_COLOR  = RGB(128, 128, 128);

	constexpr uint32_t BitColor[2] = { 0xFF000000, 0xFFFFFFFF};
	constexpr uint32_t TASKBAR_HEIGHT = 26;

	constexpr int COLOR_THRESHOLD = 50;  // 颜色识别误差阈值，距离黑色或白色小于此值视为有效


}
