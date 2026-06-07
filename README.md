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
