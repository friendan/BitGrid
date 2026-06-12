#include "ScreenSelectOverlay.hpp"

#include <Windows.h>
#include <windowsx.h>
#include <algorithm>
#include <cstdint>
#include <cstring>

struct OverlayState
{
    RECT virtualRc{};
    POINT start{};
    POINT cur{};
    bool dragging = false;
    bool done = false;
    bool cancelled = false;
    RECT result{};

    HBITMAP dib = nullptr;
    HDC memdc = nullptr;
    void* bits = nullptr;
    int w = 0;
    int h = 0;
};

static void NormalizeRect(RECT& rc)
{
    if (rc.left > rc.right) std::swap(rc.left, rc.right);
    if (rc.top > rc.bottom) std::swap(rc.top, rc.bottom);
}

static void ClearSurface(OverlayState* st)
{
    if (!st || !st->bits) return;
    // 半透明黑色遮罩 + 绿色边框（更像截图工具）
    const uint8_t a = 80; // 0=全透明, 255=不透明
    const size_t pxCount = size_t(st->w) * size_t(st->h);
    uint8_t* p = (uint8_t*)st->bits;
    for (size_t i = 0; i < pxCount; ++i) {
        p[i * 4 + 0] = 0;   // B
        p[i * 4 + 1] = 0;   // G
        p[i * 4 + 2] = 0;   // R
        p[i * 4 + 3] = a;   // A
    }
}

static void DrawBorderRGBA(OverlayState* st, const RECT& rcLocal, int thickness)
{
    if (!st || !st->bits) return;
    RECT r = rcLocal;
    NormalizeRect(r);
    auto clampi = [](LONG v, int lo, int hi) -> int {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return (int)v;
    };
    r.left = clampi(r.left, 0, st->w);
    r.right = clampi(r.right, 0, st->w);
    r.top = clampi(r.top, 0, st->h);
    r.bottom = clampi(r.bottom, 0, st->h);

    if (r.right - r.left <= 1 || r.bottom - r.top <= 1) return;

    auto put = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= st->w || y >= st->h) return;
        // BGRA
        uint8_t* p = (uint8_t*)st->bits + (size_t(y) * size_t(st->w) + size_t(x)) * 4;
        p[0] = 0;     // B
        p[1] = 0;     // G
        p[2] = 0;     // R
        p[3] = 255;   // A
    };

    for (int t = 0; t < thickness; ++t) {
        int left = r.left + t;
        int right = r.right - 1 - t;
        int top = r.top + t;
        int bottom = r.bottom - 1 - t;
        if (left >= right || top >= bottom) break;
        for (int x = left; x <= right; ++x) {
            put(x, top);
            put(x, bottom);
        }
        for (int y = top; y <= bottom; ++y) {
            put(left, y);
            put(right, y);
        }
    }
}

static void PresentLayered(HWND hwnd, OverlayState* st)
{
    HDC hdcScreen = GetDC(nullptr);
    SIZE size{ st->w, st->h };
    POINT src{ 0,0 };
    POINT dst{ st->virtualRc.left, st->virtualRc.top };
    BLENDFUNCTION bf{};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;
    UpdateLayeredWindow(hwnd, hdcScreen, &dst, &size, st->memdc, &src, 0, &bf, ULW_ALPHA);
    ReleaseDC(nullptr, hdcScreen);
}

static POINT GetLocalCursorPoint(OverlayState* st)
{
    POINT ptScreen{};
    GetCursorPos(&ptScreen);
    POINT ptLocal{};
    ptLocal.x = ptScreen.x - st->virtualRc.left;
    ptLocal.y = ptScreen.y - st->virtualRc.top;
    return ptLocal;
}

static LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* st = (OverlayState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (msg) {
    case WM_CREATE:
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE && st) {
            st->cancelled = true;
            st->done = true;
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_RBUTTONDOWN:
        if (st) {
            st->cancelled = true;
            st->done = true;
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_LBUTTONDOWN:
        if (st) {
            SetCapture(hwnd);
            POINT pt = GetLocalCursorPoint(st);
            st->start = pt;
            st->cur = pt;
            st->dragging = true;
            ClearSurface(st);
            PresentLayered(hwnd, st);
        }
        return 0;
    case WM_MOUSEMOVE:
        if (st && st->dragging) {
            POINT pt = GetLocalCursorPoint(st);
            st->cur = pt;
            ClearSurface(st);
            RECT rc{ st->start.x, st->start.y, st->cur.x, st->cur.y };
            NormalizeRect(rc);
            DrawBorderRGBA(st, rc, 4);
            PresentLayered(hwnd, st);
        }
        return 0;
    case WM_LBUTTONUP:
        if (st && st->dragging) {
            ReleaseCapture();
            st->dragging = false;
            POINT pt = GetLocalCursorPoint(st);
            st->cur = pt;

            RECT rcLocal{ st->start.x, st->start.y, st->cur.x, st->cur.y };
            NormalizeRect(rcLocal);
            // 转屏幕坐标：overlay 左上即虚拟屏幕左上
            st->result.left = st->virtualRc.left + rcLocal.left;
            st->result.top = st->virtualRc.top + rcLocal.top;
            st->result.right = st->virtualRc.left + rcLocal.right;
            st->result.bottom = st->virtualRc.top + rcLocal.bottom;

            st->done = true;
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_DESTROY:
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

static bool InitSurface(HWND hwnd, OverlayState* st)
{
    if (!st) return false;
    st->virtualRc.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    st->virtualRc.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    st->virtualRc.right = st->virtualRc.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    st->virtualRc.bottom = st->virtualRc.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
    st->w = st->virtualRc.right - st->virtualRc.left;
    st->h = st->virtualRc.bottom - st->virtualRc.top;
    if (st->w <= 0 || st->h <= 0) return false;

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = st->w;
    bi.bmiHeader.biHeight = -st->h; // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(nullptr);
    st->memdc = CreateCompatibleDC(hdcScreen);
    st->dib = CreateDIBSection(hdcScreen, &bi, DIB_RGB_COLORS, &st->bits, nullptr, 0);
    ReleaseDC(nullptr, hdcScreen);

    if (!st->memdc || !st->dib || !st->bits) return false;
    SelectObject(st->memdc, st->dib);
    ClearSurface(st);
    PresentLayered(hwnd, st);
    return true;
}

bool ScreenSelectOverlay::SelectRect(RECT& outRcScreen)
{
    OverlayState st{};

    WNDCLASSW wc{};
    wc.lpfnWndProc = OverlayProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"BitGridSelectOverlay";
    wc.hCursor = LoadCursorW(nullptr, IDC_CROSS);
    RegisterClassW(&wc);

    st.virtualRc.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    st.virtualRc.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    st.virtualRc.right = st.virtualRc.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    st.virtualRc.bottom = st.virtualRc.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        L"",
        WS_POPUP,
        st.virtualRc.left,
        st.virtualRc.top,
        st.virtualRc.right - st.virtualRc.left,
        st.virtualRc.bottom - st.virtualRc.top,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );
    if (!hwnd) return false;

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)&st);
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);

    if (!InitSurface(hwnd, &st)) {
        DestroyWindow(hwnd);
        return false;
    }

    MSG msg{};
    while (!st.done && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (IsWindow(hwnd)) {
        DestroyWindow(hwnd);
    }

    if (st.memdc) DeleteDC(st.memdc);
    if (st.dib) DeleteObject(st.dib);

    if (st.cancelled) return false;

    RECT r = st.result;
    NormalizeRect(r);
    if (r.right - r.left <= 1 || r.bottom - r.top <= 1) return false;

    outRcScreen = r;
    return true;
}

// ==================== 持续显示选择框 ====================

static HWND g_borderHwnd = nullptr;
static HBITMAP g_borderDib = nullptr;
static HDC g_borderMemdc = nullptr;
static void* g_borderBits = nullptr;
static int g_borderW = 0, g_borderH = 0;
static RECT g_borderRect{};

static void DrawBorderOnlyRGBA(void* bits, int w, int h, const RECT& rcLocal, int thickness)
{
    if (!bits) return;
    // 清空为全透明
    memset(bits, 0, size_t(w) * h * 4);
    
    RECT r = rcLocal;
    auto clampi = [](LONG v, int lo, int hi) -> int {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return (int)v;
    };
    r.left = clampi(r.left, 0, w);
    r.right = clampi(r.right, 0, w);
    r.top = clampi(r.top, 0, h);
    r.bottom = clampi(r.bottom, 0, h);
    
    if (r.right - r.left <= 1 || r.bottom - r.top <= 1) return;
    
    auto put = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        uint8_t* p = (uint8_t*)bits + (size_t(y) * w + size_t(x)) * 4;
        p[0] = 0;     // B
        p[1] = 0;     // G
        p[2] = 0;     // R
        p[3] = 255;   // A
    };
    
    for (int t = 0; t < thickness; ++t) {
        int left = r.left + t;
        int right = r.right - 1 - t;
        int top = r.top + t;
        int bottom = r.bottom - 1 - t;
        if (left >= right || top >= bottom) break;
        for (int x = left; x <= right; ++x) {
            put(x, top);
            put(x, bottom);
        }
        for (int y = top; y <= bottom; ++y) {
            put(left, y);
            put(right, y);
        }
    }
}

static LRESULT CALLBACK BorderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        
        if (g_borderMemdc && g_borderHwnd) {
            // 直接从 DC 更新分层窗口
            HDC hdcScreen = GetDC(nullptr);
            SIZE size{ g_borderW, g_borderH };
            POINT src{ 0, 0 };
            POINT dst{ g_borderRect.left, g_borderRect.top };
            BLENDFUNCTION bf{};
            bf.BlendOp = AC_SRC_OVER;
            bf.SourceConstantAlpha = 255;
            bf.AlphaFormat = AC_SRC_ALPHA;
            UpdateLayeredWindow(hwnd, hdcScreen, &dst, &size, g_borderMemdc, &src, 0, &bf, ULW_ALPHA);
            ReleaseDC(nullptr, hdcScreen);
        }
        
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        g_borderHwnd = nullptr;
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void ScreenSelectOverlay::ShowBorder(const RECT& rcScreen)
{
    HideBorder();
    
    g_borderRect = rcScreen;
    
    WNDCLASSW wc{};
    wc.lpfnWndProc = BorderWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"BitGridBorderOverlay";
    RegisterClassW(&wc);
    
    g_borderHwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        wc.lpszClassName,
        L"",
        WS_POPUP,
        rcScreen.left,
        rcScreen.top,
        rcScreen.right - rcScreen.left,
        rcScreen.bottom - rcScreen.top,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );
    if (!g_borderHwnd) return;
    
    int w = rcScreen.right - rcScreen.left;
    int h = rcScreen.bottom - rcScreen.top;
    if (w <= 0 || h <= 0) { DestroyWindow(g_borderHwnd); g_borderHwnd = nullptr; return; }
    
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    
    HDC hdcScreen = GetDC(nullptr);
    g_borderMemdc = CreateCompatibleDC(hdcScreen);
    g_borderDib = CreateDIBSection(hdcScreen, &bi, DIB_RGB_COLORS, &g_borderBits, nullptr, 0);
    ReleaseDC(nullptr, hdcScreen);
    
    if (!g_borderMemdc || !g_borderDib || !g_borderBits) {
        HideBorder();
        return;
    }
    SelectObject(g_borderMemdc, g_borderDib);
    
    g_borderW = w;
    g_borderH = h;
    
    // 计算边框的区域（相对于窗口左上角），框住整个窗口边缘
    RECT borderLocal{ 0, 0, w, h };
    DrawBorderOnlyRGBA(g_borderBits, w, h, borderLocal, 4);
    
    ShowWindow(g_borderHwnd, SW_SHOW);
    
    // 强制刷新
    InvalidateRect(g_borderHwnd, nullptr, TRUE);
    UpdateWindow(g_borderHwnd);
}

void ScreenSelectOverlay::HideBorder()
{
    if (g_borderMemdc) { DeleteDC(g_borderMemdc); g_borderMemdc = nullptr; }
    if (g_borderDib) { DeleteObject(g_borderDib); g_borderDib = nullptr; }
    g_borderBits = nullptr;
    if (g_borderHwnd) { DestroyWindow(g_borderHwnd); g_borderHwnd = nullptr; }
}

void ScreenSelectOverlay::UpdateBorder(const RECT& rcScreen)
{
    if (g_borderHwnd) {
        HideBorder();
    }
    ShowBorder(rcScreen);
}

