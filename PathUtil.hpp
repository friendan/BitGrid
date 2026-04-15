#pragma once

#include <Windows.h>
#include <string>

namespace PathUtil
{
    // exe 所在目录（不含末尾反斜杠）
    std::wstring GetExeDir();

    // yyyymmdd
    std::wstring GetTodayFolderName();

    // <exeDir>\yyyymmdd （不保证存在）
    std::wstring GetTodayFolderPath();

    // 创建目录（递归），成功返回 true
    bool EnsureDirExists(const std::wstring& dir);

    // 删除目录（递归），返回是否成功（不存在也算成功）
    bool RemoveDirRecursive(const std::wstring& dir, std::wstring* errMsg = nullptr);

    // 返回 <dir>\N.png （N 从 1 递增，扫描当前目录最大序号+1）
    std::wstring NextPngPathInDir(const std::wstring& dir);

    // 过滤 Windows 非法文件名字符；为空时返回 fallbackName
    std::wstring SanitizeFileName(const std::wstring& name, const std::wstring& fallbackName);
}

