# BitGrid
BitGrid

使用分层窗口（Layered Window）
创建一个透明覆盖层窗口，专门用于显示像素数据
✅ 由 DWM 管理，不会被覆盖
✅ 支持透明，可以看到下层窗口
✅ 始终在最上层（WS_EX_TOPMOST）
✅ 窗口移动时自动跟随

DIB 像素数据 → UpdateLayeredWindow → DWM 合成 → 屏幕
                ↓
        直接传递像素，无渲染管线处理
		
		使用分层窗口是最佳方案：
✅ 颜色 100% 精确（不会失真）
✅ 持久显示（DWM 管理，不会被覆盖）
✅ 性能良好（直接像素传递）
✅ 支持透明叠加（可以看到下层窗口）
✅ 跟随窗口移动（自动更新位置）

https://github.com/forrestchang/andrej-karpathy-skills
https://github.com/RapidAI/RapidOCR
https://onnxruntime.ai
https://modelscope.cn/models/RapidAI/RapidOCR/tree/master/mnn

模型字典文件下载位置(在官方源码目录的位置)：
https://github.com/PaddlePaddle/PaddleOCR/tree/release/3.5/ppocr/utils/dict
ppocrv5_dict.txt

## 区域选择功能（2026-06-05）

BitGrid.exe 工具栏上有两个区域选择按钮：

### 1. 选择窗口区域（btnSelect）
- 按钮文本："选择窗口区域"
- 用鼠标拖拽框选屏幕上任意矩形区域
- 选择结果存入 `selectedRectScreen`（`RECT` 类型）
- `hasSelection` 标记是否已选择
- 供"截图"、"识别"等功能使用

### 2. 选择状态栏区域（btnSelectStatusBar）
- 按钮文本："选择状态栏区域"
- 功能和选择窗口区域相同，选择结果独立存储
- 选择结果存入 `selectedStatusBarRect`（`RECT` 类型）
- `hasStatusBarSelection` 标记是否已选择
- 预留用于后续新功能

### 相关文件
- `main.htm` — 按钮布局定义
- `main.cpp` — 事件处理逻辑

## 经验教训（2026-06-29）

### 1. `std::ostringstream` 对超过 2GB 数据的截断问题

**现象**：还原 1.35GB 的 7z 文件时，还原出的文件无法解压。排查发现 `mHexString` 只有 2.147GB，远小于文件所需的 2.716GB。

**根因**：`AppUtil::GetFileDrawHexString` 中使用 `std::ostringstream` 拼接文件十六进制字符串和协议头时，MSVC 的 `stringbuf` 内部缓冲区扩容上限硬编码为 `INT_MAX`（2,147,483,647）。当拼接数据超过此值时，后续数据被静默丢弃。

**关键代码**（MSVC `sstream` 源码）：
```cpp
// stringbuf::overflow 中的扩容逻辑
if (_Oldsize < INT_MAX / 2) {
    _Newsize = _Oldsize << 1;      // 小于1GB时翻倍扩容
} else if (_Oldsize < INT_MAX) {
    _Newsize = INT_MAX;            // 超过1GB时上限锁定为INT_MAX
}
```

**后果**：
- 小于 1GB 的文件（hex < 2GB）正常工作
- 大于 1GB 的文件（hex > 2GB）被截断，且无任何错误提示
- `mTotalPage` 基于截断后的数据计算，页数偏少
- 截断点正好在 `INT_MAX`（2,147,483,647），排查时极易被误认为是 32 位 int 溢出

**修复**：将 `ostringstream` 替换为 `std::string` 直接拼接。
```cpp
// 旧代码（有截断风险）：
std::ostringstream oss;
oss << nameLenHex << totalPageHex << fileNameHexStr << contentLenHex << fileHexStr;
std::string result = oss.str();

// 新代码（安全）：
std::string result = nameLenHex + totalPageHex + fileNameHexStr + contentLenHex;
result += fileHexStr;
```

**教训**：
- 不要假设标准库组件没有隐藏限制，`ostringstream` 的简洁是以隐藏风险为代价的
- 处理可能超过 GB 级的数据时，优先用 `std::string` 直接拼接
- 小数据测试通过不代表大数据也正常，边界测试很重要
- 有疑点时写最小复现代码验证，比翻源码或猜测更高效
- 关键操作在关键节点记录数据量日志，便于排查
