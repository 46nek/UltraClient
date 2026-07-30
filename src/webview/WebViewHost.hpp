#pragma once
// ============================================================
// WebViewHost.hpp — WebView2 ホスト層
// ============================================================
// 仕様書 §5-1, §6-3 に基づく実装
//
// 責務:
//   - WebView2環境の初期化 (GPU引数・ハードウェアアクセラレーション強制)
//   - krunker.io のロード・ナビゲーション管理
//   - WebResourceRequested による広告ブロック (ResourceFilter連携)
//   - NavigationCompleted 後のJS注入 (オーバーレイ・ゲームプリセット)
//   - ホスト↔WebView2 間のメッセージング (PostWebMessageAsJson)
//   - CSS/JSファイルの自動スキャン・注入
// ============================================================

#include <windows.h>
#include <wrl/client.h>
#include <WebView2.h>
#include <string>
#include <functional>
#include <vector>

namespace webview {

// ============================================================
// WebView2ホスト設定
// ============================================================
struct WebViewConfig {
    HWND         hwnd           = nullptr;
    std::wstring userDataFolder;
    std::wstring uiResourcePath;
    bool         hardwareAccel              = true;
    bool         disableGpuVsync           = true;
    bool         ignoreGpuBlocklist        = false;
    bool         disableBackgroundThrottling = true;
    bool         uncapFps                  = true;
    bool         mouseFlickFix             = true;
    bool         blockAds                  = false;
    bool         enableSwapper             = true;
    bool         enableUserscripts         = true;
    std::wstring swapperDir;
    std::wstring scriptsDir;
};

// ============================================================
// WebView2 ホストクラス
// ============================================================
class WebViewHost {
public:
    static WebViewHost& Instance();

    // 非同期初期化 (コールバックで完了通知)
    // @param config        設定
    // @param onReady       初期化完了コールバック
    void InitAsync(const WebViewConfig& config, std::function<void(bool success)> onReady);

    // krunker.io をロード
    void Navigate(const std::wstring& url = L"https://krunker.io");

    // WebView2のサイズをウィンドウに合わせてリサイズ
    void Resize(int width, int height);

    // JavaScriptを実行 (非同期)
    void ExecuteScript(const std::wstring& script);

    // キャッシュとメモリの即時解放
    void ClearCacheAndMemory();

    // C++ -> JS のメッセージ送信
    void SendWebMessage(const std::wstring& msg);

    // ファイルからJSを読み込んで実行
    void ExecuteScriptFile(const std::wstring& filePath);

    // ファイルからCSSを読み込んでWebViewに注入
    void InjectCssFile(const std::wstring& filePath);

    // resources/ui/js/ 以下の全JSファイルを注入
    void InjectAllUiScripts();

    // resources/ui/css/ 以下の全CSSファイルを注入
    void InjectAllUiStyles();

    // ユーザースクリプトの読み込みと注入 (resources/userscripts)
    void LoadUserScripts();

    // C++ → WebView2 メッセージ送信 (JSON)
    void PostMessage(const std::string& jsonStr);

    // WebView2 → C++ メッセージ受信コールバック設定
    void SetMessageCallback(std::function<void(const std::wstring&)> cb) {
        m_messageCallback = std::move(cb);
    }

    bool IsReady() const { return m_ready; }
    
    // CoreWebView2 インスタンスの取得 (機能拡張用)
    ICoreWebView2* GetWebView() const { return m_webview.Get(); }

    // コピー禁止
    WebViewHost(const WebViewHost&)            = delete;
    WebViewHost& operator=(const WebViewHost&) = delete;

private:
    WebViewHost() = default;

    // WebView2 COM オブジェクト
    Microsoft::WRL::ComPtr<ICoreWebView2Environment>  m_environment;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller>   m_controller;
    Microsoft::WRL::ComPtr<ICoreWebView2>             m_webview;

    WebViewConfig m_config;
    bool          m_ready = false;

    std::function<void(const std::wstring&)> m_messageCallback;

    // イベント登録
    void RegisterEvents();

    // 追加ブラウザ引数を構築 (GPU/FPS最大化)
    std::wstring BuildBrowserArguments() const;

    // ファイル一覧取得ユーティリティ
    static std::vector<std::wstring> ListFiles(
        const std::wstring& directory, const std::wstring& extension);

    // ファイル内容読み込み
    static std::wstring ReadFileContent(const std::wstring& filePath);

    // CSSをJavaScript経由で注入するスクリプトを生成
    static std::wstring BuildCssInjectionScript(const std::wstring& css);
};

} // namespace webview
