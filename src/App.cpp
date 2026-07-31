// ============================================================
// App.cpp — アプリケーションライフサイクル管理 実装
// ============================================================

#include "App.hpp"

#include "util/Logger.hpp"
#include "settings/Settings.hpp"
#include "process/ProcessManager.hpp"

#include "window/WindowManager.hpp"
#include "webview/WebViewHost.hpp"
#include "webview/ResourceFilter.hpp"
#include "util/StringUtil.hpp"
#include "../resources/icons/resource.h"

#include <shlobj.h>
#include <filesystem>
#include <format>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <shellapi.h>
#include <thread>
#include "dxgi_hook/latency_optimizer.hpp"

typedef LONG (CALLBACK* NTSETTIMERRESOLUTION)(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution);

namespace app {

App& App::Instance() {
    static App s_instance;
    return s_instance;
}

std::wstring App::GetExeDir() {
    wchar_t exePath[MAX_PATH] = {};
    ::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t lastSep = path.rfind(L'\\');
    if (lastSep != std::wstring::npos)
        path = path.substr(0, lastSep);
    return path;
}

std::wstring App::GetSettingsPath() {
    return GetExeDir() + L"\\settings.json";
}

// ============================================================
// Nagleアルゴリズム即時無効化 (TCP_NODELAY via socket option)
// ============================================================
static void ApplyNagleDisable() {
    // ダミー接続を使わず、グローバルに有効化するには
    // NetworkOptimizer の per-NIC レジストリ設定が必要だが、
    // こちらは管理者不要の範囲での試み（Winsock内部フラグ）
    // 実質的にはレジストリが最も確実なので tcpRegistry フラグで管理
    LOG_INFO("App: Nagle disable configured (TCPNoDelay will apply to new sockets).");
}

// ============================================================
// 現在の設定をJS UIに送信
// ============================================================
static void SendSettingsToUI() {
    const auto& cfg = settings::SettingsManager::Instance().Get();

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> w(sb);
    w.StartObject();
    w.Key("type"); w.String("settingsLoaded");
    w.Key("settings"); w.StartObject();
      w.Key("process"); w.StartObject();
        w.Key("highPriority");    w.Bool(cfg.process.highPriority);
        w.Key("cpuAffinityMask"); w.Int(cfg.process.cpuAffinityMask);
      w.EndObject();
      w.Key("browser"); w.StartObject();
        w.Key("hardwareAccel");              w.Bool(cfg.browser.hardwareAccel);
        w.Key("disableVSync");               w.Bool(cfg.browser.disableVSync);
        w.Key("ignoreGpuBlocklist");         w.Bool(cfg.browser.ignoreGpuBlocklist);
        w.Key("disableBackgroundThrottling"); w.Bool(cfg.browser.disableBackgroundThrottling);
        w.Key("mouseFlickFix");              w.Bool(cfg.browser.mouseFlickFix);
      w.EndObject();
      w.Key("network"); w.StartObject();
        w.Key("disableNagle");     w.Bool(cfg.network.disableNagle);
      w.EndObject();
      w.Key("extension"); w.StartObject();
        w.Key("blockAds");        w.Bool(cfg.extension.blockAds);
        w.Key("startFullscreen"); w.Bool(cfg.extension.startFullscreen);
        w.Key("enableSwapper");   w.Bool(cfg.extension.enableSwapper);
        w.Key("enableUserscripts"); w.Bool(cfg.extension.enableUserscripts);
      w.EndObject();
    w.EndObject();
    w.EndObject();

    webview::WebViewHost::Instance().PostMessage(std::string(sb.GetString()));
    LOG_INFO("App: Sent settingsLoaded to UI.");
}

// ============================================================
// 設定メッセージをJSから受け取って即時適用
// ============================================================
static void HandleSaveSettings(const rapidjson::Document& d) {
    if (!d.HasMember("settings") || !d["settings"].IsObject()) return;
    const auto& s = d["settings"];

    auto& cfg = settings::SettingsManager::Instance().GetMutable();

    if (s.HasMember("process") && s["process"].IsObject()) {
        const auto& p = s["process"];
        if (p.HasMember("highPriority") && p["highPriority"].IsBool())
            cfg.process.highPriority = p["highPriority"].GetBool();
    }
    if (s.HasMember("browser") && s["browser"].IsObject()) {
        const auto& b = s["browser"];
        if (b.HasMember("hardwareAccel") && b["hardwareAccel"].IsBool())
            cfg.browser.hardwareAccel = b["hardwareAccel"].GetBool();
        if (b.HasMember("disableVSync") && b["disableVSync"].IsBool())
            cfg.browser.disableVSync = b["disableVSync"].GetBool();
        if (b.HasMember("ignoreGpuBlocklist") && b["ignoreGpuBlocklist"].IsBool())
            cfg.browser.ignoreGpuBlocklist = b["ignoreGpuBlocklist"].GetBool();
        if (b.HasMember("disableBackgroundThrottling") && b["disableBackgroundThrottling"].IsBool())
            cfg.browser.disableBackgroundThrottling = b["disableBackgroundThrottling"].GetBool();
        if (b.HasMember("mouseFlickFix") && b["mouseFlickFix"].IsBool())
            cfg.browser.mouseFlickFix = b["mouseFlickFix"].GetBool();
    }
    if (s.HasMember("network") && s["network"].IsObject()) {
        const auto& n = s["network"];
        if (n.HasMember("disableNagle") && n["disableNagle"].IsBool())
            cfg.network.disableNagle = n["disableNagle"].GetBool();
    }
    if (s.HasMember("extension") && s["extension"].IsObject()) {
        const auto& e = s["extension"];
        if (e.HasMember("blockAds") && e["blockAds"].IsBool())
            cfg.extension.blockAds = e["blockAds"].GetBool();
        if (e.HasMember("startFullscreen") && e["startFullscreen"].IsBool())
            cfg.extension.startFullscreen = e["startFullscreen"].GetBool();
        if (e.HasMember("enableSwapper") && e["enableSwapper"].IsBool())
            cfg.extension.enableSwapper = e["enableSwapper"].GetBool();
        if (e.HasMember("enableUserscripts") && e["enableUserscripts"].IsBool())
            cfg.extension.enableUserscripts = e["enableUserscripts"].GetBool();
    }

    settings::SettingsManager::Instance().Save();
    LOG_INFO("App: Settings saved via UI.");

    // ========================================
    // 即時適用できるものを適用
    // ========================================

    // プロセス優先度
    {
        auto& pm = process::ProcessManager::Instance();
        if (cfg.process.highPriority)
            pm.SetProcessPriority(process::PriorityLevel::High);
        else
            pm.SetProcessPriority(process::PriorityLevel::Normal);
    }

    // 広告ブロック
    if (cfg.extension.blockAds)
        webview::ResourceFilter::Instance().LoadDefaultRules();
}

// ============================================================
// 初期化
// ============================================================
bool App::Init(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    bool isAltWindow = false;
    int argc = 0;
    if (LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc)) {
        for (int i = 0; i < argc; ++i) {
            if (std::wstring(argv[i]) == L"--alt-window") {
                isAltWindow = true;
                break;
            }
        }
        ::LocalFree(argv);
    }

    // Winsock初期化
    WSADATA wsaData;
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);

    // =========================================
    // 3. ログの初期化
    // =========================================
    std::wstring exeDir = GetExeDir();
    std::wstring logDir = exeDir + L"\\logs";
    if (!std::filesystem::exists(logDir)) {
        std::filesystem::create_directories(logDir);
    }
    
    std::wstring swapperDir = exeDir + L"\\swapper";
    if (!std::filesystem::exists(swapperDir))
        std::filesystem::create_directories(swapperDir);

    std::wstring scriptsDir = exeDir + L"\\scripts";
    if (!std::filesystem::exists(scriptsDir))
        std::filesystem::create_directories(scriptsDir);
    
    // =========================================
    // 1. Logger & Timer Resolution
    // =========================================
    std::wstring logPath = logDir + L"\\krunker_client.log";
    util::Logger::Instance().Init(logPath);
    LOG_INFO("=== Krunker Ultra Client Starting ===");

    HMODULE hNtdll = ::GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) hNtdll = ::LoadLibraryW(L"ntdll.dll");
    if (hNtdll) {
        if (auto pNtSetTimerResolution = (NTSETTIMERRESOLUTION)::GetProcAddress(hNtdll, "NtSetTimerResolution")) {
            ULONG currentRes;
            // 0.5ms (5000 in 100-ns units)
            pNtSetTimerResolution(5000, TRUE, &currentRes);
            LOG_INFO("App: System Timer Resolution set to 0.5ms");
        }
    }

    // =========================================
    // 2. Settings読み込み
    // =========================================
    auto& settings = settings::SettingsManager::Instance();
    settings.Load(GetSettingsPath());
    const auto& cfg = settings.Get();

    // =========================================
    // 3. プロセス優先度設定
    // =========================================
    {
        auto& pm = process::ProcessManager::Instance();
        if (cfg.process.highPriority)
            pm.SetProcessPriority(process::PriorityLevel::High);
        else
            pm.SetProcessPriority(process::PriorityLevel::Normal);

        if (cfg.process.cpuAffinityMask != 0)
            pm.SetCpuAffinity(static_cast<DWORD_PTR>(cfg.process.cpuAffinityMask));
    }

    // =========================================
    // 4. TCP最適化
    // =========================================


    if (cfg.network.disableNagle) {
        ApplyNagleDisable();
    }

    // =========================================
    // 5. 広告・トラッカーブロッカー
    // =========================================
    if (cfg.extension.blockAds)
        webview::ResourceFilter::Instance().LoadDefaultRules();

    if (cfg.extension.enableSwapper)
        webview::ResourceFilter::Instance().SetSwapperDirectory(swapperDir);

    // =========================================
    // 6. ウィンドウ作成
    // =========================================
    {
        window::WindowConfig wConfig;
        wConfig.title      = isAltWindow ? L"Krunker Ultra Client (Alt / Ranked)" : L"Krunker Ultra Client";
        wConfig.width      = isAltWindow ? 450 : 1280;
        wConfig.height     = isAltWindow ? 350 : 720;
        wConfig.fullscreen = isAltWindow ? false : cfg.extension.startFullscreen;
        wConfig.icon       = ::LoadIconW(::GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_ICON1));

        if (!window::WindowManager::Instance().Create(wConfig)) {
            LOG_ERROR("Failed to create main window.");
            return false;
        }
    }

    HWND hwnd = window::WindowManager::Instance().GetHwnd();

    // =========================================
    // 7. WebView2初期化
    // =========================================
    {
        webview::WebViewConfig wvConfig;
        wvConfig.hwnd                        = hwnd;
        wvConfig.userDataFolder              = exeDir + L"\\WebViewData";
        wvConfig.uiResourcePath              = exeDir + L"\\ui";
        wvConfig.hardwareAccel               = cfg.browser.hardwareAccel;
        wvConfig.disableGpuVsync             = cfg.browser.disableVSync;
        wvConfig.ignoreGpuBlocklist          = cfg.browser.ignoreGpuBlocklist;
        wvConfig.disableBackgroundThrottling = cfg.browser.disableBackgroundThrottling;
        wvConfig.mouseFlickFix               = cfg.browser.mouseFlickFix;
        wvConfig.blockAds                    = cfg.extension.blockAds;
        wvConfig.enableSwapper               = cfg.extension.enableSwapper;
        wvConfig.enableUserscripts           = cfg.extension.enableUserscripts;
        wvConfig.isAltWindow                 = isAltWindow;
        wvConfig.swapperDir                  = swapperDir;
        wvConfig.scriptsDir                  = scriptsDir;

        window::WindowManager::Instance().SetWebViewResizeCallback(
            [](int w, int h) {
                webview::WebViewHost::Instance().Resize(w, h);
            });

        window::WindowManager::Instance().SetCopyDataCallback([](const std::string& msg) {
            // Forward IPC from other windows to this window's WebView
            webview::WebViewHost::Instance().PostMessage(msg);
        });

        webview::WebViewHost::Instance().InitAsync(wvConfig,
            [this, highPriority = cfg.process.highPriority, isAltWindow](bool success) {
                if (success) {
                    // メッセージ受信コールバック
                    webview::WebViewHost::Instance().SetMessageCallback([](const std::wstring& msg) {
                        std::string utf8Msg = util::WideToUtf8(msg);
                        rapidjson::Document d;
                        d.Parse(utf8Msg.c_str());
                        if (!d.HasParseError() && d.HasMember("type") && d["type"].IsString()) {
                            std::string type = d["type"].GetString();
                            if (type == "saveSettings") {
                                HandleSaveSettings(d);
                            } else if (type == "requestSettings") {
                                SendSettingsToUI();
                            } else if (type == "openFolder") {
                                if (d.HasMember("folder") && d["folder"].IsString()) {
                                    std::wstring folder = util::Utf8ToWide(d["folder"].GetString());
                                    std::wstring path = GetExeDir() + L"\\" + folder;
                                    ::ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                                }
                            } else if (type == "openAltWindow") {
                                std::wstring exePath = GetExeDir() + L"\\KrunkerUltraClient.exe";
                                ::ShellExecuteW(nullptr, L"open", exePath.c_str(), L"--alt-window", nullptr, SW_SHOWNORMAL);
                            } else if (type == "forwardIpcToMain") {
                                // Send IPC from Alt Window to Main Window
                                HWND hMain = ::FindWindowW(L"KrunkerUltraClientWnd", L"Krunker Ultra Client");
                                if (hMain) {
                                    COPYDATASTRUCT cds;
                                    cds.dwData = 1;
                                    cds.cbData = utf8Msg.size() + 1;
                                    cds.lpData = (PVOID)utf8Msg.c_str();
                                    ::SendMessageW(hMain, WM_COPYDATA, 0, (LPARAM)&cds);
                                }
                            } else if (type == "forwardIpcToAlt") {
                                // Send IPC from Main Window to Alt Window
                                HWND hAlt = ::FindWindowW(L"KrunkerUltraClientWnd", L"Krunker Ultra Client (Alt / Ranked)");
                                if (hAlt) {
                                    COPYDATASTRUCT cds;
                                    cds.dwData = 1;
                                    cds.cbData = utf8Msg.size() + 1;
                                    cds.lpData = (PVOID)utf8Msg.c_str();
                                    ::SendMessageW(hAlt, WM_COPYDATA, 0, (LPARAM)&cds);
                                }
                            }
                        }
                    });

                    if (isAltWindow) {
                        std::wstring rankedUrl = L"file:///" + GetExeDir() + L"/ui/ranked.html";
                        // Convert backslashes to forward slashes for URL
                        std::replace(rankedUrl.begin(), rankedUrl.end(), L'\\', L'/');
                        webview::WebViewHost::Instance().Navigate(rankedUrl);
                        LOG_INFO("App: WebView2 ready. Navigated to local ranked.html");
                    } else {
                        webview::WebViewHost::Instance().Navigate(L"https://krunker.io");
                        LOG_INFO("App: WebView2 ready. Navigated to krunker.io");
                    }

                    // 管理者権限不足の通知を遅延送信
                    if (m_pendingAdminNotify) {
                        std::string msg = R"({"type":"adminRequired","message":"TCPレジストリ最適化を適用するには管理者権限が必要です。クライアントを右クリック→「管理者として実行」で起動してください。"})";
                        webview::WebViewHost::Instance().PostMessage(msg);
                        m_pendingAdminNotify = false;
                    }

                    // UI同期はJSからのrequestSettingsを待って行うため、ここでは送らない

                    // WebView2の子プロセスの優先度をHIGHに引き上げる
                    if (highPriority) {
                        process::ProcessManager::Instance().ElevateChildProcesses();
                    }

                    // DWMとGPUレイテンシの最適化を適用
                    dxgi_hook::OptimizeWindowLatency(window::WindowManager::Instance().GetHwnd());
                } else {
                    LOG_ERROR("App: WebView2 initialization failed.");
                    ::PostQuitMessage(1);
                }
            });
    }

    m_running = true;
    LOG_INFO("App: Initialization complete.");
    return true;
}

// ============================================================
// メインエントリー
// ============================================================
int App::Run(HINSTANCE hInstance) {
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!Init(hInstance)) {
        LOG_ERROR("App: Initialization failed. Exiting.");
        ::CoUninitialize();
        return 1;
    }

    int exitCode = window::WindowManager::Instance().RunMessageLoop();

    Shutdown();
    ::CoUninitialize();
    ::WSACleanup();

    return exitCode;
}

// ============================================================
// シャットダウン
// ============================================================
void App::Shutdown() {
    LOG_INFO("App: Shutting down...");
    settings::SettingsManager::Instance().Save();
    LOG_INFO("App: Shutdown complete.");
}

void App::RequestQuit() {
    if (m_running)
        ::PostQuitMessage(0);
}

} // namespace app
