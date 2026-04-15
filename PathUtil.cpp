#include "PathUtil.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

static std::wstring TrimTrailingSlash(std::wstring s)
{
    while (!s.empty() && (s.back() == L'\\' || s.back() == L'/')) {
        s.pop_back();
    }
    return s;
}

std::wstring PathUtil::GetExeDir()
{
    wchar_t buf[MAX_PATH]{};
    DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring full(buf, buf + len);
    auto pos = full.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return L".";
    return TrimTrailingSlash(full.substr(0, pos));
}

std::wstring PathUtil::GetTodayFolderName()
{
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t dateBuf[16]{};
    swprintf_s(dateBuf, L"%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
    return dateBuf;
}

std::wstring PathUtil::GetTodayFolderPath()
{
    return GetExeDir() + L"\\" + GetTodayFolderName();
}

bool PathUtil::EnsureDirExists(const std::wstring& dir)
{
    std::error_code ec;
    fs::create_directories(fs::path(dir), ec);
    return !ec;
}

bool PathUtil::RemoveDirRecursive(const std::wstring& dir, std::wstring* errMsg)
{
    std::error_code ec;
    const fs::path p(dir);
    if (!fs::exists(p, ec)) {
        return true; // 不存在也算成功
    }
    fs::remove_all(p, ec);
    if (ec) {
        if (errMsg) {
            *errMsg = L"remove_all failed, code=" + std::to_wstring(ec.value());
        }
        return false;
    }
    return true;
}

static bool TryParseInt(const std::wstring& s, int& out)
{
    if (s.empty()) return false;
    int v = 0;
    for (wchar_t c : s) {
        if (c < L'0' || c > L'9') return false;
        v = v * 10 + (c - L'0');
    }
    out = v;
    return true;
}

std::wstring PathUtil::NextPngPathInDir(const std::wstring& dir)
{
    int maxN = 0;
    std::error_code ec;
    const fs::path p(dir);
    if (fs::exists(p, ec) && fs::is_directory(p, ec)) {
        for (auto& it : fs::directory_iterator(p, ec)) {
            if (ec) break;
            if (!it.is_regular_file(ec)) continue;
            fs::path fp = it.path();
            auto ext = fp.extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
            if (ext != L".png") continue;
            int n = 0;
            if (TryParseInt(fp.stem().wstring(), n)) {
                if (n > maxN) maxN = n;
            }
        }
    }
    int next = maxN + 1;
    return TrimTrailingSlash(dir) + L"\\" + std::to_wstring(next) + L".png";
}

std::wstring PathUtil::SanitizeFileName(const std::wstring& name, const std::wstring& fallbackName)
{
    std::wstring s = name;
    // 去掉路径分隔符与非法字符
    const std::wstring illegal = L"<>:\"/\\|?*";
    for (auto& ch : s) {
        if (illegal.find(ch) != std::wstring::npos) ch = L'_';
    }
    // 不能以空格/点结尾（Windows 兼容）
    while (!s.empty() && (s.back() == L' ' || s.back() == L'.')) s.pop_back();
    // 不能是空
    if (s.empty()) return fallbackName;
    return s;
}

