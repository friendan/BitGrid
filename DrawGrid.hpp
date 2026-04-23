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
	
	// 为分层窗口初始化绘制参数
	void DrawInitForDIB(int width, int height);
	void DrawBorder(HWND hwnd, HDC hdc);
	void DrawHexString(HWND hwnd, HDC hdc);
	void DrawPixGrid(HWND hwnd);
	void DrawPixGridToOverlay(HWND hwndOverlay, HBITMAP hBitmap, uint32_t* pPixels, int width, int height);
	
	// 直接在 DIB 像素数组上绘制边框（用于分层窗口）
	void DrawBorderToDIB(uint32_t* pPixels, int width, int height);
	
	// 直接在 DIB 像素数组上绘制十六进制数据（用于分层窗口）
	void DrawHexStringToDIB(uint32_t* pPixels, int width, int height);

	void SetHexString(const std::string& hexString);
	
	// 设置带文件名和文件长度的数据（新格式）
	// 格式：文件名长度(4字节) + 文件名(256字节) + 文件内容长度(4字节) + 文件内容
	void SetFileData(const std::string& fileName, const std::string& fileContentHex);
	
	const std::string& GetHexString() const { return mHexString; }
	int GetCurPage() const { return mCurPage; }
	void NextPage();
	void ChangePage(int chVal);
	
	void InitGdiPlus();
	void UninitGdiPlus();

	static COLORREF ColorToRGB(const Gdiplus::Color& color);

	// 从截图还原十六进制字符串
	// 单页情况：传入图片路径，返回还原的十六进制字符串
	// 输出参数：outFileName 返回文件名，outFileContentHex 返回文件内容的十六进制
	static std::string RestoreFromImage(const std::wstring& imagePath, 
	                                     std::string* outFileName = nullptr,
	                                     std::string* outFileContentHex = nullptr,
	                                     bool isFirstPage = true);
	// 多页情况：传入文件夹路径，返回还原的十六进制字符串（按文件创建时间排序）
	// 输出参数：outFileName 返回文件名，outFileContentHex 返回文件内容的十六进制
	static std::string RestoreFromFolder(const std::wstring& folderPath,
	                                      std::string* outFileName = nullptr,
	                                      std::string* outFileContentHex = nullptr);

public:
	size_t mWidth = 0;
	size_t mHeight = 0;
	size_t mTotalPage = 0;
	size_t mPageSize  = 0;
	size_t mCurPage   = 1; // 从1开始
	size_t mDrawWidth = 0;
    size_t mDrawHeight = 0;
    // std::string mHexString = "0123456789ABCDEF";
    std::string mHexString;

private:
    // GDI+ 全局变量
    Gdiplus::GdiplusStartupInput gdiplusStartupInput{};
    ULONG_PTR gdiplusToken = NULL;
    
    
};
