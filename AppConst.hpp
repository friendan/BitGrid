#pragma once
#include <Windows.h>

namespace AppConst
{
	// 二进制值(0~15) → 16进制字符
	constexpr char BinTohex[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	constexpr int BORDER_LINE_OFFSET = 5;	// 表示：从顶部/底部/左侧/右侧 数第几条线
	constexpr int BORDER_LINE_COUNT  = 3;	// 表示：从顶部/底部/左侧/右侧 依次画N条线
	constexpr COLORREF BORDER_COLOR  = RGB(0, 0, 0);	// 黑色边框
	// constexpr COLORREF BORDER_COLOR  = RGB(255, 0, 0);	// 红色边框（旧）
	constexpr COLORREF BACKGROUND_COLOR  = RGB(128, 128, 128);
	constexpr COLORREF COLOR_BLACK = RGB(0, 0, 0);
	constexpr COLORREF COLOR_WHITE = RGB(255, 255, 255);

	constexpr uint32_t BitColor[2] = { 0xFF000000, 0xFFFFFFFF};
	constexpr uint32_t TASKBAR_HEIGHT = 26;
	constexpr int COLOR_THRESHOLD = 350;  // 颜色识别误差阈值，降低阈值避免灰色背景被误识别（远程桌面环境下背景色失真为RGB(121,121,121)到黑色距离363）


}
