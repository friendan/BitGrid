#include "AppUtil.hpp"
#include "AppConst.hpp"
#include <cuchar>
#include <stdexcept>
#include <clocale>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <filesystem>
namespace fs = std::filesystem;

size_t AppUtil::DrawFileSize = 0;

uint8_t AppUtil::HexCharToBits(char hexChar)
{
	uint8_t value = 0;

    // 转换为 0~15 的值
    if (hexChar >= '0' && hexChar <= '9') {
        value = hexChar - '0';
    } else if (hexChar >= 'A' && hexChar <= 'F') {
        value = 10 + (hexChar - 'A');
    } else if (hexChar >= 'a' && hexChar <= 'f') {
        value = 10 + (hexChar - 'a');
    }
    return value;   // 返回的 value 本身就是4位二进制值
}

void AppUtil::HexCharToBits(char hexChar, uint8_t bits[4]){
	uint8_t bin = AppUtil::HexCharToBits(hexChar);
	// 下标 0~3 直接对应 二进制第0位~第3位
	bits[0] =  bin        & 1;  // 第0位（最低位）
	bits[1] = (bin >> 1)  & 1;  // 第1位
	bits[2] = (bin >> 2)  & 1;  // 第2位
	bits[3] = (bin >> 3)  & 1;  // 第3位（最高位）
}

char AppUtil::BitsToHexChar(uint8_t bits[4])
{
    uint8_t val = 0;
    val |= (bits[0] & 0x01) << 0;	// 把 bits[0]（第0位）放到 val 的第0位
    val |= (bits[1] & 0x01) << 1;	// 把 bits[1]（第1位）放到 val 的第1位  
    val |= (bits[2] & 0x01) << 2;	// 把 bits[2]（第2位）放到 val 的第2位
    val |= (bits[3] & 0x01) << 3;	// 把 bits[3]（第3位）放到 val 的第3位
    return (val < 10) ? ('0' + val) : ('A' + val - 10);	  // 将合并后的数值（0~15）转换为16进制字符并返回
}

std::string AppUtil::WStrToStr(const std::wstring& wstr){
	if (wstr.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);
    if (!result.empty()) result.pop_back();
    return result;
}

std::wstring AppUtil::StrToWStr(const std::string& str){
	if (str.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], len);
    if (!result.empty()) result.pop_back();
    return result;
}

std::string AppUtil::ReadFileToHexString(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
       return {};
    }
    size_t fileSize = fs::file_size(path);

    // 一次性读取全部数据（比流方式快非常多）
    std::string binaryData;
    binaryData.resize(static_cast<size_t>(fileSize));
    file.read(&binaryData[0], fileSize);

    // 【高性能】预分配输出内存（1字节 = 2个十六进制字符）
    std::string hexString;
    hexString.reserve(static_cast<size_t>(fileSize) * 2);

    // 十六进制字符表（查表法，最快）
    static const char hexTable[] = "0123456789ABCDEF";

    // 纯内存操作，无任何格式化开销 → 速度极快
    for (uint8_t byte : binaryData)
    {
        hexString += hexTable[byte >> 4];    // 高4位
        hexString += hexTable[byte & 0x0F];  // 低4位
    }
    return hexString;
}

std::string AppUtil::ReadFileToHexString(const std::wstring& path)
{
    std::string strFilePath = AppUtil::WStrToStr(path);
    return AppUtil::ReadFileToHexString(strFilePath);
}

std::string AppUtil::StrToHexStr(const std::string& str){
    if(str.empty()) return "";
    std::string hexString;
    hexString.reserve(static_cast<size_t>(str.size()) * 2);

    // 十六进制字符表（查表法，最快）
    static const char hexTable[] = "0123456789ABCDEF";

    // 纯内存操作，无任何格式化开销 → 速度极快
    for (uint8_t byte : str)
    {
        hexString += hexTable[byte >> 4];    // 高4位
        hexString += hexTable[byte & 0x0F];  // 低4位
    }
    return hexString;
}

bool AppUtil::WriteHexStringToFile(const std::string& hexStr, const std::string& strFilePath)
{
    if (hexStr.empty()) {
        return false;
    }

    // 如果长度是奇数，丢弃最后一个字符（不完整的字节）
    std::string normalizedHex = hexStr;
    if (normalizedHex.size() % 2 != 0) {
        normalizedHex = hexStr.substr(0, hexStr.size() - 1);
        SaveLog("[WriteHexStringToFile] Truncated odd-length hex string: ", 
                std::to_string(hexStr.size()), " -> ", std::to_string(normalizedHex.size()));
    }

    std::ofstream file(strFilePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // 预分配二进制内存（2个十六进制字符 = 1字节）
    const size_t byteCount = normalizedHex.size() / 2;
    std::string binaryData;
    binaryData.reserve(byteCount);

    // 高性能查表转换（不使用字符串流，纯内存操作）
    auto CharToHex = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        return 0xFF; // 非法字符
    };

    // 每两个字符合成一个字节
    for (size_t i = 0; i < normalizedHex.size(); i += 2) {
        uint8_t high = CharToHex(normalizedHex[i]);
        uint8_t low = CharToHex(normalizedHex[i + 1]);
        binaryData += (high << 4) | low;
    }

    // 一次性写入文件（极快）
    file.write(binaryData.data(), std::streamsize(binaryData.size()));
    return true;
}

bool AppUtil::WriteHexStringToFile(const std::string& hexStr, const std::wstring& wstrFilePath)
{
    return AppUtil::WriteHexStringToFile(hexStr, WStrToStr(wstrFilePath));
}

std::string AppUtil::GetSubStrByPage(const std::string& str, size_t pageSize, size_t pageNum){
    if(str.empty() || pageSize < 1 || pageNum < 1){
        return "";
    }
    size_t totalPages = (str.size() + pageSize - 1) / pageSize;
    if(pageNum > totalPages){
        return "";
    }
    size_t startPos = (pageNum - 1)*pageSize;
    size_t endPos = startPos + pageSize;
    if(endPos > str.size()){
        endPos = str.size();
    }
    return str.substr(startPos, endPos - startPos);
}

std::string_view AppUtil::GetSubStrViewByPage(const std::string& str, size_t pageSize, size_t pageNum){
    if(str.empty() || pageSize < 1 || pageNum < 1){
        return std::string_view();
    }
    size_t totalPages = (str.size() + pageSize - 1) / pageSize;
    if(pageNum > totalPages){
        return std::string_view();
    }
    size_t startPos = (pageNum - 1)*pageSize;
    size_t endPos = startPos + pageSize;
    if(endPos > str.size()){
        endPos = str.size();
    }
    return std::string_view(str.data() + startPos, endPos - startPos);
}

std::string AppUtil::GetTimeStr(){
    time_t now = time(nullptr);
    tm t{};
    localtime_s(&t, &now); // Windows下安全的本地时间函数
    char buf[32] = {0};
    sprintf_s(buf, "[%04d-%02d-%02d %02d:%02d:%02d]", 
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
             t.tm_hour, t.tm_min, t.tm_sec);
    return buf;
}

static std::mutex g_log_mutex;
static std::string g_log_filename;

// 获取日志文件名（基于exe文件名）
static std::string get_log_filename() {
    if (!g_log_filename.empty()) {
        return g_log_filename;
    }
    
    // 获取exe路径
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    
    // 提取exe文件名（不含扩展名）
    std::wstring wExePath(exePath);
    std::wstring wFileName = wExePath.substr(wExePath.find_last_of(L"/\\") + 1);
    std::wstring wNameWithoutExt = wFileName.substr(0, wFileName.find_last_of(L"."));
    
    // 转换为窄字符并添加.log扩展名
    g_log_filename = AppUtil::WStrToStr(wNameWithoutExt) + ".log";
    return g_log_filename;
}

void write_log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_log_mutex); // 自动加锁/解锁
    std::string log_file_name = get_log_filename();
    std::ofstream log_file(log_file_name, std::ios::app | std::ios::out);
    if (log_file.is_open()) {
        log_file << AppUtil::GetTimeStr() << " " << msg << std::endl;
        log_file.close();
    }
}

void AppUtil::SaveLog(const std::string& msg){
    write_log(msg);
}

void AppUtil::SaveLog(const std::wstring& msg){
    write_log(WStrToStr(msg));
}

std::string AppUtil::OpenFileDialog(HWND hParent){
    char szFilePath[MAX_PATH] = { 0 };
    OPENFILENAMEA ofn = { 0 };
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = hParent;
    ofn.lpstrFile = szFilePath;
    ofn.nMaxFile = MAX_PATH;
    // 文件类型过滤器
    // ofn.lpstrFilter = "所有文件(*.*)\0*.*\0文本文件(*.txt)\0*.txt\0";
    ofn.lpstrFilter = "所有文件(*.*)\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY; // 必须存在文件、路径，隐藏只读选项
    if (GetOpenFileNameA(&ofn))
    {
        return std::string(szFilePath);
    }
    return "";
}

std::string AppUtil::GetFileDrawHexString(HWND hParent){
    std::string filePath = AppUtil::OpenFileDialog(hParent);
    if(filePath.empty()) return "";

    AppUtil::DrawFileSize = fs::file_size(filePath);
    std::string fileHexStr = AppUtil::ReadFileToHexString(filePath);
    if(fileHexStr.empty()){
        AppUtil::SaveLog("fileHexStr is empty: ", filePath);
        return "";
    }

    std::string fileName = fs::path(filePath).filename().string();
    std::string fileName256 = AppUtil::StringToLen256(fileName);
    std::string fileNameHexStr = AppUtil::StrToHexStr(fileName256);
    std::ostringstream oss;
    oss << fileNameHexStr << fileHexStr;

    // AppUtil::SaveLog("filePath ", filePath);
    AppUtil::SaveLog("fileName ", fileName);
    AppUtil::SaveLog("fileName256 ", fileName256);
    AppUtil::SaveLog("fileNameHexStr ", fileNameHexStr);
    // AppUtil::SaveLog("fileHexStr ", fileHexStr);

    return oss.str();
}

 std::string AppUtil::StringToLen256(const std::string& str){
    static size_t len = 256;
    std::string res;
    res.reserve(len);
    if (str.size() >= len) {
        res = str.substr(0, len);
    } else {
        res = str;
        res.append(len - str.size(), '0');
    }
    return res;
 }

std::string AppUtil::HexStrToStr(const std::string& hexStr)
{
    if (hexStr.empty() || hexStr.length() % 2 != 0) {
        return "";
    }

    std::string result;
    result.reserve(hexStr.length() / 2);

    // 高性能查表转换
    auto CharToHex = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        return 0;
    };

    for (size_t i = 0; i < hexStr.length(); i += 2) {
        uint8_t high = CharToHex(hexStr[i]);
        uint8_t low = CharToHex(hexStr[i + 1]);
        result += static_cast<char>((high << 4) | low);
    }

    return result;
}

bool AppUtil::IsRgbColor(COLORREF rgbColor1, COLORREF rgbColor2)
{
    // 分别取出 R、G、B 分量
    int r1 = GetRValue(rgbColor1);
    int g1 = GetGValue(rgbColor1);
    int b1 = GetBValue(rgbColor1);

    int r2 = GetRValue(rgbColor2);
    int g2 = GetGValue(rgbColor2);
    int b2 = GetBValue(rgbColor2);

    // 计算三个通道的差值总和（曼哈顿距离）
    int dist = abs(r1 - r2) + abs(g1 - g2) + abs(b1 - b2);

    // 在阈值内就认为颜色相同
    return dist <= AppConst::COLOR_THRESHOLD;
}

uint8_t AppUtil::GetRgbColorBit(COLORREF rgbColor){
    if(AppUtil::IsRgbColor(rgbColor, AppConst::COLOR_BLACK)){
       return 0;
    }
    if(AppUtil::IsRgbColor(rgbColor, AppConst::COLOR_WHITE)){
       return 1;
    }
    return 255; // error
}

