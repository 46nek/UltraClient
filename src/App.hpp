#pragma once
// ============================================================
// App.hpp — アプリケーションライフサイクル管理
// ============================================================
// 各サブシステムを統括し、起動順序と終了処理を管理する。
//
// 起動順序:
//   1. Logger 初期化
//   2. Settings 読み込み
//   3. ProcessManager — プロセス/スレッド優先度設定
//   4. NetworkOptimizer — TCP最適化 (設定に応じて)
//   5. WindowManager — ウィンドウ作成
//   6. InputHandler — Raw Input 登録
//   7. WebViewHost — WebView2 初期化・krunker.io ロード
//   8. PingMeasurer — 並列ping計測・監視開始
// ============================================================

#include <windows.h>
#include <string>

namespace app {

class App {
public:
    static App& Instance();

    // 初期化・メインループ実行・クリーンアップを一括実行
    // 戻り値: プロセス終了コード
    int Run(HINSTANCE hInstance);

    // アプリ終了要求
    void RequestQuit();

    // コピー禁止
    App(const App&)            = delete;
    App& operator=(const App&) = delete;

private:
    App() = default;

    bool Init(HINSTANCE hInstance);
    void Shutdown();

    // 実行ファイルと同じディレクトリのパスを返す
    static std::wstring GetExeDir();

    // 設定ファイルのパスを返す
    static std::wstring GetSettingsPath();

    HINSTANCE m_hInstance        = nullptr;
    bool      m_running          = false;
    bool      m_pendingAdminNotify = false;
};

} // namespace app
