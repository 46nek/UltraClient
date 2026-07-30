#pragma once
// ============================================================
// Settings.hpp — アプリケーション設定管理
// ============================================================
// クライアント特有の軽量化・最適化設定に特化
// 永続化: JSON (RapidJSON) → settings.json
// ============================================================

#include <string>
#include <cstdint>

namespace settings {

// ============================================================
// プロセス設定 (即時適用)
// ============================================================
struct ProcessConfig {
    bool highPriority    = true;  // HIGH_PRIORITY_CLASS
    int  cpuAffinityMask = 0;     // 0=自動
};

// ============================================================
// ブラウザエンジン設定 (再起動後に適用)
// ============================================================
struct BrowserConfig {
    bool hardwareAccel              = true;   // GPUアクセラレーション
    bool disableVSync               = true;   // GPU垂直同期オフ
    bool ignoreGpuBlocklist         = false;  // GPUブラックリスト無視
    bool disableBackgroundThrottling = true;  // 非アクティブ時FPS低下防止
    bool mouseFlickFix              = true;   // 高ポーリングレートマウスのフリーズ修正
};

// ============================================================
// ネットワーク設定
// ============================================================
struct NetworkConfig {
    bool disableNagle     = true;   // TCP_NODELAY (即時適用)
    bool applyTcpRegistry = false;  // TCPレジストリ最適化 (管理者権限必須)
};

// ============================================================
// 拡張・UI設定
// ============================================================
struct ExtensionConfig {
    bool blockAds        = false;  // 広告・トラッカーブロック (デフォルトオフ)
    bool startFullscreen = false;  // フルスクリーン起動
    bool enableSwapper   = true;   // リソーススワッパー
    bool enableUserscripts = true; // ユーザースクリプト
};

// ============================================================
// アプリケーション全設定
// ============================================================
struct AppSettings {
    ProcessConfig   process;
    BrowserConfig   browser;
    NetworkConfig   network;
    ExtensionConfig extension;

    std::wstring settingsFilePath;
};

// ============================================================
// 設定管理クラス (シングルトン)
// ============================================================
class SettingsManager {
public:
    static SettingsManager& Instance();

    bool Load(const std::wstring& filePath);
    bool Save() const;

    const AppSettings& Get()        const { return m_settings; }
    AppSettings&       GetMutable()       { return m_settings; }
    void               Apply(const AppSettings& s) { m_settings = s; }

    SettingsManager(const SettingsManager&)            = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

private:
    SettingsManager() = default;
    AppSettings m_settings;
};

} // namespace settings
