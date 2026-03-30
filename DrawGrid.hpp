#pragma once
#include <Windows.h>
#include <cstdint>  // 必须包含，用于uint8_t
#include <string>
#include <gdiplus.h>

class DrawGrid
{
private:
	DrawGrid() {}
public:
	static DrawGrid* Inst(){
		static DrawGrid inst;
		return &inst;
	}

    // 在窗口客户区绘制边界线（顶部、底部、左侧、右侧）
	void DrawInit(HWND hwnd, HDC hdc);
	void DrawBorder(HWND hwnd, HDC hdc);
	void DrawHexString(HWND hwnd, HDC hdc);
	void DrawPixGrid(HWND hwnd);

	void SetHexString(const std::string& hexString);
	void NextPage();
	void ChangePage(int chVal);
	
	void InitGdiPlus();
	void UninitGdiPlus();

public:
	size_t mWidth = 0;
	size_t mHeight = 0;
	size_t mTotalPage = 0;
	size_t mPageSize  = 0;
	size_t mCurPage   = 1; // 从1开始
	size_t mDrawWidth = 0;
    size_t mDrawHeight = 0;
    std::string mHexString = "0123456789ABCDEF";

private:
    // GDI+ 全局变量
    Gdiplus::GdiplusStartupInput gdiplusStartupInput{};
    ULONG_PTR gdiplusToken = NULL;
    
    
};
