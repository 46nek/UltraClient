#pragma once
// ============================================================
// WindowManager.hpp — Win32ウィンドウ管理
// ============================================================
// - ウィンドウ生成・メッセージループ
// - フルスクリーン / ウィンドウモード切り替え
// - DPI対応 (Windows 10/11)
// - WM_INPUT を InputHandler へ転送
// ============================================================

#include <windows.h>
#include <string>
#include <functional>

namespace window {

// ============================================================
// ウィンドウ設定
// ============================================================
struct WindowConfig {
    std::wstring title       = L"Krunker Ultra Client";
    int          width       = 1280;
    int          height      = 720;
    bool         fullscreen  = false;
    bool         topmost     = false;   // 最前面固定 (将来対応)
    HICON        icon        = nullptr;
};

// ============================================================
// ウィンドウ管理クラス (シングルトン)
// ============================================================
class WindowManager {
public:
    static WindowManager& Instance();

    // ウィンドウを作成・表示
    bool Create(const WindowConfig& config);

    // メッセージループを実行 (アプリ終了までブロック)
    int RunMessageLoop();

    // フルスクリーン / ウィンドウモード切り替え
    void ToggleFullscreen();
    void SetFullscreen(bool fullscreen);
    bool IsFullscreen() const { return m_isFullscreen; }

    // リサイズコールバック
    void SetWebViewResizeCallback(std::function<void(int, int)> cb) { m_resizeCb = cb; }

    // IPC(WM_COPYDATA)コールバック
    void SetCopyDataCallback(std::function<void(const std::string&)> cb) { m_copyDataCb = cb; }

    // ウィンドウハンドル取得
    HWND GetHwnd() const { return m_hwnd; }

    // クライアント領域サイズ取得
    void GetClientSize(int& width, int& height) const;

    // WebView2が占有する領域をウィンドウサイズに同期
    void ResizeWebView();

    // スプラッシュスクリーンの表示・非表示
    void ShowSplashScreen();
    void HideSplashScreen();

    // コピー禁止
    WindowManager(const WindowManager&)            = delete;
    WindowManager& operator=(const WindowManager&) = delete;

private:
    WindowManager() = default;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND     m_hwnd         = nullptr;
    bool             m_isFullscreen = false;
    DWORD            m_savedStyle   = 0;
    WINDOWPLACEMENT  m_savedPlacement = {};

    std::function<void(int, int)> m_resizeCb;
    std::function<void(const std::string&)> m_copyDataCb;

    HBITMAP          m_hSplashBmp = nullptr;
    HWND             m_hSplash    = nullptr;
};

} // namespace window
