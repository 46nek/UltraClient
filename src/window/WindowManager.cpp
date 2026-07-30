// ============================================================
// WindowManager.cpp — Win32ウィンドウ管理 実装
// ============================================================

#include "WindowManager.hpp"
#include "util/Logger.hpp"
#include <shellapi.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")
#include <vector>

namespace window {

// ウィンドウクラス名
static constexpr const wchar_t* kClassName = L"KrunkerUltraClientWnd";

// ============================================================
// WindowProc — Win32メッセージハンドラ
// ============================================================
LRESULT CALLBACK WindowManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WindowManager& wm = WindowManager::Instance();

    switch (msg) {

    case WM_CREATE:
        return 0;

    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (wm.m_resizeCb && wParam != SIZE_MINIMIZED) {
            wm.m_resizeCb(w, h);
        }
        return 0;
    }

    // (Removed WM_INPUT routing to InputHandler to prevent conflicts with WebView2 Pointer Lock)

    // F11 でフルスクリーントグル
    case WM_KEYDOWN:
        if (wParam == VK_F11) {
            wm.ToggleFullscreen();
        }
        return 0;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    // DPI変更への対応
    case WM_DPICHANGED: {
        const RECT* pRect = reinterpret_cast<const RECT*>(lParam);
        ::SetWindowPos(hwnd, nullptr,
            pRect->left, pRect->top,
            pRect->right  - pRect->left,
            pRect->bottom - pRect->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

    default:
        return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

// ============================================================
// シングルトン
// ============================================================
WindowManager& WindowManager::Instance() {
    static WindowManager s_instance;
    return s_instance;
}

// ============================================================
// ウィンドウ作成
// ============================================================
bool WindowManager::Create(const WindowConfig& config) {
    // DPI認識を設定 (Windows 10 Anniversary Update以降)
    ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HINSTANCE hInstance = ::GetModuleHandleW(nullptr);

    // ウィンドウクラス登録
    WNDCLASSEXW wc       = {};
    wc.cbSize            = sizeof(wc);
    wc.style             = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc       = WndProc;
    wc.hInstance         = hInstance;
    wc.hCursor           = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground     = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
    wc.lpszClassName     = kClassName;
    wc.hIcon             = config.icon;
    wc.hIconSm           = config.icon;

    if (!::RegisterClassExW(&wc)) {
        DWORD err = ::GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            LOG_ERROR("WindowManager: RegisterClassExW failed. Error=" + std::to_string(err));
            return false;
        }
    }

    // ウィンドウスタイル
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (config.fullscreen) {
        style = WS_POPUP;
    }

    // クライアント領域がconfig.width x config.heightになるよう調整
    RECT rc = { 0, 0, config.width, config.height };
    ::AdjustWindowRect(&rc, style, FALSE);

    // ウィンドウ作成
    m_hwnd = ::CreateWindowExW(
        0,
        kClassName,
        config.title.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right  - rc.left,
        rc.bottom - rc.top,
        nullptr, nullptr,
        hInstance,
        nullptr
    );

    if (!m_hwnd) {
        LOG_ERROR("WindowManager: CreateWindowExW failed. Error=" + std::to_string(::GetLastError()));
        return false;
    }

    // DWM ダークモード有効化 (Windows 10 1809以降)
    BOOL darkMode = TRUE;
    ::DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

    if (config.fullscreen) {
        SetFullscreen(true);
    }
    
    ::ShowWindow(m_hwnd, SW_SHOW);
    ::UpdateWindow(m_hwnd);

    // スプラッシュスクリーンは非表示 (ユーザー要望により)
    // ShowSplashScreen();

    LOG_INFO("WindowManager: Window created (" +
             std::to_string(config.width) + "x" + std::to_string(config.height) + ")");
    return true;
}

// ============================================================
// メッセージループ
// ============================================================
int WindowManager::RunMessageLoop() {
    MSG msg = {};
    while (::GetMessageW(&msg, nullptr, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

// ============================================================
// フルスクリーン切り替え
// ============================================================
void WindowManager::ToggleFullscreen() {
    SetFullscreen(!m_isFullscreen);
}

void WindowManager::SetFullscreen(bool fullscreen) {
    if (m_isFullscreen == fullscreen) return;

    if (fullscreen) {
        // 現在の状態を保存
        m_savedStyle = ::GetWindowLongW(m_hwnd, GWL_STYLE);
        ::GetWindowPlacement(m_hwnd, &m_savedPlacement);

        // フルスクリーン化: ボーダーなし + モニター全域に拡大
        HMONITOR monitor = ::MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi   = { sizeof(mi) };
        ::GetMonitorInfoW(monitor, &mi);

        ::SetWindowLongW(m_hwnd, GWL_STYLE,
            m_savedStyle & ~(WS_CAPTION | WS_THICKFRAME));

        ::SetWindowPos(m_hwnd, HWND_TOP,
            mi.rcMonitor.left,  mi.rcMonitor.top,
            mi.rcMonitor.right  - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else {
        // ウィンドウモードに復元
        ::SetWindowLongW(m_hwnd, GWL_STYLE, m_savedStyle);
        ::SetWindowPlacement(m_hwnd, &m_savedPlacement);
        ::SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }

    m_isFullscreen = fullscreen;
    LOG_INFO("WindowManager: Fullscreen = " + std::string(fullscreen ? "true" : "false"));
}

// ============================================================
// クライアント領域サイズ
// ============================================================
void WindowManager::GetClientSize(int& width, int& height) const {
    RECT rc = {};
    ::GetClientRect(m_hwnd, &rc);
    width  = rc.right  - rc.left;
    height = rc.bottom - rc.top;
}

void WindowManager::ResizeWebView() {
    if (m_resizeCb) {
        int w, h;
        GetClientSize(w, h);
        m_resizeCb(w, h);
    }
    
    // スプラッシュスクリーンもセンタリングする
    if (m_hSplash && m_hSplashBmp) {
        BITMAP bm;
        if (::GetObject(m_hSplashBmp, sizeof(bm), &bm)) {
            int w, h;
            GetClientSize(w, h);
            ::SetWindowPos(m_hSplash, HWND_TOP,
                (w - bm.bmWidth) / 2, (h - bm.bmHeight) / 2,
                bm.bmWidth, bm.bmHeight, SWP_NOZORDER);
        }
    }
}

// ============================================================
// スプラッシュスクリーン
// ============================================================
void WindowManager::ShowSplashScreen() {
    if (m_hSplash) return; // 既に表示済み

    // IDB_SPLASH (102) からビットマップをロード
    m_hSplashBmp = (HBITMAP)::LoadImageW(::GetModuleHandleW(nullptr),
                                         MAKEINTRESOURCEW(102),
                                         IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (!m_hSplashBmp) {
        LOG_WARN("WindowManager: Failed to load splash bitmap (IDB_SPLASH=102)");
        return;
    }

    BITMAP bm;
    ::GetObject(m_hSplashBmp, sizeof(bm), &bm);

    int w, h;
    GetClientSize(w, h);

    m_hSplash = ::CreateWindowExW(
        0, L"STATIC", nullptr,
        WS_CHILD | WS_VISIBLE | SS_BITMAP,
        (w - bm.bmWidth) / 2, (h - bm.bmHeight) / 2,
        bm.bmWidth, bm.bmHeight,
        m_hwnd, nullptr, ::GetModuleHandleW(nullptr), nullptr
    );

    if (m_hSplash) {
        ::SendMessage(m_hSplash, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hSplashBmp);
        LOG_INFO("WindowManager: Splash screen shown.");
    }
}

void WindowManager::HideSplashScreen() {
    if (m_hSplash) {
        ::DestroyWindow(m_hSplash);
        m_hSplash = nullptr;
        LOG_INFO("WindowManager: Splash screen hidden.");
    }
    if (m_hSplashBmp) {
        ::DeleteObject(m_hSplashBmp);
        m_hSplashBmp = nullptr;
    }
}

} // namespace window
