#pragma once
#include <Windows.h>
#include <cstdint>  // 必须包含，用于uint8_t
#include <string>

class AppUtil
{
public:
	// 输入：16进制字符（0-9, A-F, a-f）
	// 输出：一个uint8_t，低4位存储二进制结果（高4位为0）
	// 例如：'A' → 0x0A → 二进制 0000 1010
	/*
		uint8_t bin = AppUtil::HexCharToBits('A'); // 调用
		// bin = 10 (0x0A)，二进制：0000 1010
		// 想取每一位直接用位运算：
		uint8_t bit0 = bin & 0x01;        // 第1位
		uint8_t bit1 = (bin >> 1) & 0x01; // 第2位
		uint8_t bit2 = (bin >> 2) & 0x01; // 第3位
		uint8_t bit3 = (bin >> 3) & 0x01; // 第4位
	*/
	static uint8_t HexCharToBits(char hexChar);
	static void HexCharToBits(char hexChar, uint8_t bits[4]);


	/***********************************************************
	 * 功能：将4位二进制数组 转换为 对应的16进制字符（反向转换函数）
	 * 输入：uint8_t bits[4]
	 *       bits[0] = 二进制第0位（最低位）
	 *       bits[1] = 二进制第1位
	 *       bits[2] = 二进制第2位
	 *       bits[3] = 二进制第3位（最高位）
	 *
	 * 返回值：char 类型的16进制字符（0-9，A-F）
	 **********************************************************/
	static char BitsToHexChar(uint8_t bits[4]);


	static std::string WStrToStr(const std::wstring& wstr);
	static std::wstring StrToWStr(const std::string& str);

	/**
	 * @brief 读取文件的二进制内容，并转换为十六进制字符串
	 * @param path 文件路径
	 * @return 二进制数据对应的大写十六进制字符串（无空格，如："12AB340A0F123A"）
	 */
	static std::string ReadFileToHexString(const std::string& path);
	static std::string ReadFileToHexString(const std::wstring& path);

	static std::string StrToHexStr(const std::string& str);
	
	 /**
     * @brief 将十六进制字符串 → 写入二进制文件
     * @param hexStr 十六进制字符串（大写/小写均可，无空格）
     * @param strFilePath 要保存的文件路径
     * @return 成功返回 true，失败返回 false
     */
    static bool WriteHexStringToFile(const std::string& hexStr, const std::string& strFilePath);
    static bool WriteHexStringToFile(const std::string& hexStr, const std::wstring& wstrFilePath);

    /**
     * @brief 按分页方式获取子字符串
     * @param str 原始字符串
     * @param pageSize 每页字符数
     * @param pageNum  也码(从1开始)
     * @return 返回指定页的字符串
     */
    static std::string GetSubStrByPage(const std::string& str, size_t pageSize, size_t pageNum);

	static std::string GetTimeStr();
    static void SaveLog(const std::string& msg);
    static void SaveLog(const std::wstring& msg);

    // template <typename T>
    // void SaveLog(const T& value) {
    //     std::stringstream ss;
    //     ss << value;
    //     SaveLog(ss.str());
    // }

    // 变参模板：支持任意数量、任意类型的参数
    template <typename... Args>
    static void SaveLog(const Args&... args) {
        std::stringstream ss;
        // 折叠表达式：依次将所有参数写入stringstream（C++17+）
        (ss << ... << args);
        // 调用基础重载保存日志
        SaveLog(ss.str());
    }

    // 宽字符变参模板（显式指定宽字符参数）
    // 模板参数后缀W，明确区分宽字符版本
    template <typename... Args>
    static void SaveLogW(const Args&... args) {
        std::wstringstream wss; // 改用宽字符流
        (wss << ... << args);   // 折叠表达式支持wstring/int等
        SaveLog(wss.str());     // 调用宽字符基础重载
    }

    static std::string OpenFileDialog(HWND hParent);
    static std::string GetFileDrawHexString(HWND hParent);
    static size_t DrawFileSize;

    static std::string StringToLen256(const std::string& str);

    /**
     * @brief 将十六进制字符串转换为普通字符串
     * @param hexStr 十六进制字符串（如："74657374" -> "test"）
     * @return 转换后的普通字符串
     */
    static std::string HexStrToStr(const std::string& hexStr);

};
