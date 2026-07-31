// ============================================================
// WebViewHost.cpp -- WebView2 ホスト層 実装
// ============================================================

#include "WebViewHost.hpp"
#include "ResourceFilter.hpp"
#include "util/Logger.hpp"
#include "util/StringUtil.hpp"
#include "../window/WindowManager.hpp"

// WRL Callback を使うために必須
#include <wrl/event.h>
#include <wrl/implements.h>
#include <wrl/module.h>
#include <WebView2EnvironmentOptions.h>

#include <shlobj.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <algorithm>
#include <fstream>
#include <sstream>

#include "../dxgi_hook/latency_optimizer.hpp"

namespace webview {

// ============================================================
// シングルトン
// ============================================================
WebViewHost& WebViewHost::Instance() {
    static WebViewHost s_instance;
    return s_instance;
}

// ============================================================
// 追加ブラウザ引数の構築
// 仕様書 §5-1 -- GPU/FPS最大化引数
// ============================================================
std::wstring WebViewHost::BuildBrowserArguments() const {
    std::wstring args;

    if (m_config.hardwareAccel) {
        args += L"--enable-gpu-rasterization ";
        args += L"--enable-accelerated-2d-canvas ";
        args += L"--enable-webgl ";
    }

    if (m_config.disableGpuVsync) {
        args += L"--disable-gpu-vsync ";
    }

    if (m_config.ignoreGpuBlocklist) {
        args += L"--ignore-gpu-blacklist ";
    }


    if (m_config.disableBackgroundThrottling) {
        args += L"--disable-background-timer-throttling ";
        args += L"--disable-renderer-backgrounding ";
    }

    if (m_config.mouseFlickFix) {
        // ChromiumネイティブのRaw Inputエンジンを有効化 (完全な生マウス入力)
        // ※以前は無効化していましたが、視点移動の遅延を真にゼロにするためにOSレベルで有効化します
        args += L"--enable-features=PointerRawUpdate ";
    }

    // 極限軽量化フラグ (Bloat reduction)
    args += L"--disable-webrtc ";
    args += L"--disable-speech-api ";
    args += L"--disable-print-preview ";
    args += L"--disable-features=OptimizationHints,Translate ";
    
    // JSのGC(ガベージコレクション)を手動実行できるようにする
    args += L"--js-flags=\"--expose-gc\" ";

    dxgi_hook::AppendLatencyArgs(args);

    return args;
}

// ============================================================
// 非同期初期化
// ============================================================
void WebViewHost::InitAsync(const WebViewConfig& config,
                             std::function<void(bool success)> onReady) {
    m_config = config;

    // ユーザーデータフォルダ
    std::wstring userDataPath = config.userDataFolder;
    if (userDataPath.empty()) {
        PWSTR appDataPath = nullptr;
        if (SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_RoamingAppData,
                                              0, nullptr, &appDataPath))) {
            userDataPath = std::wstring(appDataPath) + L"\\KrunkerUltraClient";
            ::CoTaskMemFree(appDataPath);
        }
    }

    // ============================================================
    auto envOpts = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    
    std::wstring browserArgs = BuildBrowserArguments();
    LOG_INFO("WebViewHost: Browser args: " + util::WideToUtf8(browserArgs));

    if (envOpts) {
        envOpts->put_AdditionalBrowserArguments(browserArgs.c_str());
    } else {
        LOG_WARN("WebViewHost: Failed to create CoreWebView2EnvironmentOptions.");
    }

    // WebView2の内部制限を確実に解除するため、環境変数も設定する
    ::SetEnvironmentVariableW(L"WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", browserArgs.c_str());

    // ============================================================
    // WebView2 環境を非同期作成
    // ============================================================
    HRESULT hr = ::CreateCoreWebView2EnvironmentWithOptions(
        nullptr,              // ブラウザ実行ファイルフォルダ (nullptr=Edge標準)
        userDataPath.c_str(), // ユーザーデータフォルダ
        envOpts.Get(),        // 追加引数などのオプション

        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this, onReady, browserArgs](
                HRESULT result,
                ICoreWebView2Environment* env) -> HRESULT
            {
                if (FAILED(result) || !env) {
                    char buf[32]; snprintf(buf, sizeof(buf), "%08X", result);
                    LOG_ERROR("WebViewHost: Failed to create environment. HRESULT=0x" + std::string(buf));
                    if (onReady) onReady(false);
                    return result;
                }

                m_environment = env;
                LOG_INFO("WebViewHost: Environment created.");

                HRESULT hr2 = env->CreateCoreWebView2Controller(
                    m_config.hwnd,
                    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this, onReady](HRESULT result2, ICoreWebView2Controller* ctrl) -> HRESULT
                        {
                            if (FAILED(result2) || !ctrl) {
                                char buf[32]; snprintf(buf, sizeof(buf), "%08X", result2);
                                LOG_ERROR("WebViewHost: Failed to create controller. HRESULT=0x" + std::string(buf));
                                if (onReady) onReady(false);
                                return result2;
                            }

                            m_controller = ctrl;
                            ctrl->get_CoreWebView2(&m_webview);

                            RECT bounds{};
                            ::GetClientRect(m_config.hwnd, &bounds);
                            ctrl->put_Bounds(bounds);

                            RegisterEvents();

                            // Always inject ranked_ipc.js and ranked_launcher.js (core features)
                            if (!m_config.scriptsDir.empty()) {
                                std::wstring ipcScriptPath = m_config.scriptsDir + L"\\ranked_ipc.js";
                                std::wstring ipcScript = ReadFileContent(ipcScriptPath);
                                if (!ipcScript.empty()) {
                                    std::wstring wrapped = L"(function(){" + ipcScript + L"})();";
                                    m_webview->AddScriptToExecuteOnDocumentCreated(wrapped.c_str(), nullptr);
                                }
                                
                                std::wstring launcherPath = m_config.scriptsDir + L"\\ranked_launcher.js";
                                std::wstring launcherScript = ReadFileContent(launcherPath);
                                if (!launcherScript.empty()) {
                                    std::wstring wrapped = L"(function(){" + launcherScript + L"})();";
                                    m_webview->AddScriptToExecuteOnDocumentCreated(wrapped.c_str(), nullptr);
                                }
                            }

                            // Load userscripts via AddScriptToExecuteOnDocumentCreated
                            if (m_config.enableUserscripts && !m_config.scriptsDir.empty()) {
                                for (const auto& f : ListFiles(m_config.scriptsDir, L".js")) {
                                    if (f.find(L"ranked_overlay.js") != std::wstring::npos) continue; 
                                    if (f.find(L"ranked_ipc.js") != std::wstring::npos) continue;
                                    if (f.find(L"ranked_launcher.js") != std::wstring::npos) continue;
                                    
                                    std::wstring script = ReadFileContent(f);
                                    if (!script.empty()) {
                                        // Wrap in IIFE for scope isolation
                                        std::wstring wrapped = L"(function(){" + script + L"})();";
                                        m_webview->AddScriptToExecuteOnDocumentCreated(wrapped.c_str(), nullptr);
                                        LOG_INFO("WebViewHost: Registered userscript: " + util::WideToUtf8(f));
                                    }
                                }
                            }

                            if (m_config.isAltWindow) {
                                std::wstring script = ReadFileContent(m_config.scriptsDir + L"\\ranked_overlay.js");
                                if (!script.empty()) {
                                    std::wstring wrapped = L"(function(){" + script + L"})();";
                                    m_webview->AddScriptToExecuteOnDocumentCreated(wrapped.c_str(), nullptr);
                                    LOG_INFO("WebViewHost: Registered ranked overlay for Alt Window.");
                                }
                            }
                            m_ready = true;
                            LOG_INFO("WebViewHost: Controller ready.");

                            if (onReady) onReady(true);
                            return S_OK;
                        }
                    ).Get()
                );

                if (FAILED(hr2)) {
                    LOG_ERROR("WebViewHost: CreateCoreWebView2Controller failed.");
                    if (onReady) onReady(false);
                }
                return S_OK;
            }
        ).Get()
    );

    if (FAILED(hr)) {
        char buf[32]; snprintf(buf, sizeof(buf), "%08X", hr);
        LOG_ERROR("WebViewHost: CreateCoreWebView2EnvironmentWithOptions failed. HRESULT=0x" + std::string(buf));
        if (onReady) onReady(false);
    }
}

// ============================================================
// イベント登録
// ============================================================
void WebViewHost::RegisterEvents() {
    if (!m_webview) return;

    // ----------------------------------------
    // WebResourceRequested -- 広告ブロック・スワッパー
    // ----------------------------------------
    if (m_config.blockAds || m_config.enableSwapper) {
        // 広告ブロック: スクリプト・XHR・Fetch・画像のみ対象。
        // WebSocket (COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL) は絶対に触らない。
        // ALLを使うとゲーム中の全ネットワーク通信がこのコールバックを通り、Ping蹛ね上がりの原因になる。
        const COREWEBVIEW2_WEB_RESOURCE_CONTEXT contexts[] = {
            COREWEBVIEW2_WEB_RESOURCE_CONTEXT_SCRIPT,
            COREWEBVIEW2_WEB_RESOURCE_CONTEXT_XML_HTTP_REQUEST,
            COREWEBVIEW2_WEB_RESOURCE_CONTEXT_FETCH,
            COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE,
            COREWEBVIEW2_WEB_RESOURCE_CONTEXT_FONT,
            COREWEBVIEW2_WEB_RESOURCE_CONTEXT_MEDIA,
        };
        for (auto ctx : contexts) {
            m_webview->AddWebResourceRequestedFilter(L"*", ctx);
        }

        m_webview->add_WebResourceRequested(
            Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [this](ICoreWebView2* /*sender*/, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT
                {
                    Microsoft::WRL::ComPtr<ICoreWebView2WebResourceRequest> request;
                    args->get_Request(&request);

                    LPWSTR urlW = nullptr;
                    request->get_Uri(&urlW);
                    if (urlW) {
                        std::string url = util::WideToUtf8(urlW);
                        ::CoTaskMemFree(urlW);

                        FilterAction action = ResourceFilter::Instance().Evaluate(url);
                        if (action == FilterAction::Swap) {
                            std::wstring localPath = ResourceFilter::Instance().GetSwapFilePath(url);
                            if (!localPath.empty()) {
                                IStream* stream = nullptr;
                                HRESULT hr = ::SHCreateStreamOnFileEx(localPath.c_str(), 
                                    STGM_READ | STGM_SHARE_DENY_WRITE, 0, FALSE, nullptr, &stream);
                                if (SUCCEEDED(hr) && stream) {
                                    std::wstring mime = L"application/octet-stream";
                                    std::wstring ext = localPath.substr(localPath.find_last_of(L".") + 1);
                                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                                    if (ext == L"png") mime = L"image/png";
                                    else if (ext == L"jpg" || ext == L"jpeg") mime = L"image/jpeg";
                                    else if (ext == L"mp3") mime = L"audio/mpeg";
                                    else if (ext == L"ogg") mime = L"audio/ogg";
                                    else if (ext == L"json") mime = L"application/json";
                                    else if (ext == L"js") mime = L"application/javascript";
                                    else if (ext == L"css") mime = L"text/css";
                                    else if (ext == L"html") mime = L"text/html";
                                    else if (ext == L"obj") mime = L"model/obj";
                                    else if (ext == L"mtl") mime = L"text/plain";
                                    else if (ext == L"wav") mime = L"audio/wav";
                                    else if (ext == L"ttf") mime = L"font/ttf";
                                    else if (ext == L"woff") mime = L"font/woff";
                                    else if (ext == L"woff2") mime = L"font/woff2";
                                    else if (ext == L"otf") mime = L"font/otf";

                                    std::wstring headers = L"Content-Type: " + mime + L"\nAccess-Control-Allow-Origin: *";

                                    Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> response;
                                    m_environment->CreateWebResourceResponse(
                                        stream, 200, L"OK", headers.c_str(), &response);
                                    args->put_Response(response.Get());
                                    stream->Release();
                                }
                            }
                        } else if (action == FilterAction::Block) {
                            args->put_Response(nullptr);
                        }
                    }
                    return S_OK;
                }
            ).Get(),
            nullptr
        );

        LOG_INFO("WebViewHost: Ad/tracker blocking enabled (script/xhr/fetch/image/font only).");
    }

    // ----------------------------------------
    // NavigationCompleted -- JS/CSS注入
    // ----------------------------------------
    m_webview->add_NavigationCompleted(
        Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this](ICoreWebView2* /*sender*/, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
            {
                BOOL success = FALSE;
                args->get_IsSuccess(&success);

                if (success) {
                    LOG_INFO("WebViewHost: Navigation completed. Injecting UI scripts...");
                    InjectAllUiStyles();
                    InjectAllUiScripts();

                    // window::WindowManager::Instance().HideSplashScreen();
                } else {
                    COREWEBVIEW2_WEB_ERROR_STATUS status{};
                    args->get_WebErrorStatus(&status);
                    LOG_WARN("WebViewHost: Navigation failed. Status=" + std::to_string(status));
                }
                return S_OK;
            }
        ).Get(),
        nullptr
    );

    // ----------------------------------------
    // WebMessageReceived -- WebView2 -> C++ 通信
    // ----------------------------------------
    m_webview->add_WebMessageReceived(
        Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* /*sender*/, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
            {
                LPWSTR msg = nullptr;
                args->TryGetWebMessageAsString(&msg);
                if (msg && m_messageCallback) {
                    m_messageCallback(std::wstring(msg));
                }
                ::CoTaskMemFree(msg);
                return S_OK;
            }
        ).Get(),
        nullptr
    );

    LOG_INFO("WebViewHost: Events registered.");
}

// ============================================================
// ナビゲーション
// ============================================================
void WebViewHost::Navigate(const std::wstring& url) {
    if (!m_webview) {
        LOG_WARN("WebViewHost::Navigate called before WebView2 is ready.");
        return;
    }
    m_webview->Navigate(url.c_str());
    LOG_INFO("WebViewHost: Navigating to " + util::WideToUtf8(url));
}

// ============================================================
// リサイズ
// ============================================================
void WebViewHost::Resize(int width, int height) {
    if (!m_controller) return;
    RECT bounds = { 0, 0, width, height };
    m_controller->put_Bounds(bounds);
}

// ============================================================
// JavaScript実行
// ============================================================
void WebViewHost::ExecuteScript(const std::wstring& script) {
    if (!m_webview) return;
    m_webview->ExecuteScript(script.c_str(), nullptr);
}

void WebViewHost::ExecuteScriptFile(const std::wstring& filePath) {
    std::wstring content = ReadFileContent(filePath);
    if (!content.empty()) {
        ExecuteScript(content);
        LOG_DEBUG("WebViewHost: Injected JS: " + util::WideToUtf8(filePath));
    }
}

// ============================================================
// CSS注入
// ============================================================
void WebViewHost::InjectCssFile(const std::wstring& filePath) {
    std::wstring css = ReadFileContent(filePath);
    if (!css.empty()) {
        ExecuteScript(BuildCssInjectionScript(css));
        LOG_DEBUG("WebViewHost: Injected CSS: " + util::WideToUtf8(filePath));
    }
}

// ============================================================
// resources/ui/ 以下の全ファイルを自動注入
// CSS/JSファイルを追加するだけで自動的に読み込まれる
// ============================================================
void WebViewHost::InjectAllUiStyles() {
    if (!m_config.uiResourcePath.empty()) {
        for (const auto& f : ListFiles(m_config.uiResourcePath + L"\\css", L".css")) {
            InjectCssFile(f);
        }
    }
    
    // Inject CSS directly from swapper/ directory (これならF5リロードでも反映される)
    if (m_config.enableSwapper && !m_config.swapperDir.empty()) {
        for (const auto& f : ListFiles(m_config.swapperDir, L".css")) {
            InjectCssFile(f);
        }
    }
}

void WebViewHost::InjectAllUiScripts() {
    if (m_config.uiResourcePath.empty()) return;
    for (const auto& f : ListFiles(m_config.uiResourcePath + L"\\js", L".js")) {
        ExecuteScriptFile(f);
    }
}

void WebViewHost::LoadUserScripts() {
    if (m_config.uiResourcePath.empty()) return;
    
    std::wstring userscriptDir = m_config.uiResourcePath;
    // uiResourcePath は exeDir + "\ui" なので、exeDir + "\userscripts" を探す
    size_t pos = userscriptDir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        userscriptDir = userscriptDir.substr(0, pos) + L"\\userscripts";
    }

    for (const auto& f : ListFiles(userscriptDir, L".js")) {
        std::wstring script = ReadFileContent(f);
        if (script.empty()) continue;

        // Escape for JSON string
        std::string utf8Script = util::WideToUtf8(script);
        std::string escaped;
        for (char c : utf8Script) {
            if (c == '"') escaped += "\\\"";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else escaped += c;
        }

        std::string name = util::WideToUtf8(f.substr(f.find_last_of(L"\\/") + 1));
        std::string json = "{\"type\":\"runUserScript\",\"name\":\"" + name + "\",\"script\":\"" + escaped + "\"}";
        PostMessage(json);
        LOG_INFO("WebViewHost: Sent UserScript: " + name);
    }
}

// ============================================================
// キャッシュとメモリの即時解放
// ============================================================
void WebViewHost::ClearCacheAndMemory() {
    if (!m_webview) return;
    
    // 1. DevToolsを使ってJavaScriptのガベージコレクションを強制実行
    m_webview->CallDevToolsProtocolMethod(L"HeapProfiler.collectGarbage", L"{}", nullptr);
    m_webview->CallDevToolsProtocolMethod(L"Memory.forciblyPurgeJavaScriptMemory", L"{}", nullptr);
    
    // 2. ブラウジングデータをクリア
    Microsoft::WRL::ComPtr<ICoreWebView2_13> wv13;
    if (SUCCEEDED(m_webview.As(&wv13))) {
        Microsoft::WRL::ComPtr<ICoreWebView2Profile> profile;
        if (SUCCEEDED(wv13->get_Profile(&profile))) {
            Microsoft::WRL::ComPtr<ICoreWebView2Profile2> profile2;
            if (SUCCEEDED(profile.As(&profile2))) {
                profile2->ClearBrowsingData(
                    (COREWEBVIEW2_BROWSING_DATA_KINDS)(
                        COREWEBVIEW2_BROWSING_DATA_KINDS_CACHE_STORAGE |
                        COREWEBVIEW2_BROWSING_DATA_KINDS_DISK_CACHE |
                        COREWEBVIEW2_BROWSING_DATA_KINDS_DOWNLOAD_HISTORY |
                        COREWEBVIEW2_BROWSING_DATA_KINDS_GENERAL_AUTOFILL |
                        COREWEBVIEW2_BROWSING_DATA_KINDS_PASSWORD_AUTOSAVE
                    ),
                    nullptr
                );
            }
        }
    }
    LOG_INFO("WebViewHost: Cleared cache and forced garbage collection.");
}

// ============================================================
// メッセージ送信 (C++ -> WebView2)
// ============================================================
void WebViewHost::PostMessage(const std::string& jsonStr) {
    if (!m_webview) return;
    std::wstring wJson = util::Utf8ToWide(jsonStr);
    m_webview->PostWebMessageAsJson(wJson.c_str());
}

// ============================================================
// ユーティリティ
// ============================================================
std::vector<std::wstring> WebViewHost::ListFiles(
    const std::wstring& directory, const std::wstring& extension)
{
    std::vector<std::wstring> files;
    WIN32_FIND_DATAW fd{};
    HANDLE hFind = ::FindFirstFileW((directory + L"\\*" + extension).c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return files;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            files.push_back(directory + L"\\" + fd.cFileName);
        }
    } while (::FindNextFileW(hFind, &fd));
    ::FindClose(hFind);
    std::sort(files.begin(), files.end());
    return files;
}

std::wstring WebViewHost::ReadFileContent(const std::wstring& filePath) {
    std::ifstream ifs(filePath, std::ios::binary);
    if (!ifs.is_open()) {
        LOG_WARN("WebViewHost: Cannot open: " + util::WideToUtf8(filePath));
        return {};
    }
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    return util::Utf8ToWide(content);
}

std::wstring WebViewHost::BuildCssInjectionScript(const std::wstring& css) {
    std::wstring escaped = css;
    size_t pos = 0;
    while ((pos = escaped.find(L'\\', pos)) != std::wstring::npos) {
        escaped.replace(pos, 1, L"\\\\");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped.find(L'`', pos)) != std::wstring::npos) {
        escaped.replace(pos, 1, L"\\`");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped.find(L'$', pos)) != std::wstring::npos) {
        escaped.replace(pos, 1, L"\\$");
        pos += 2;
    }
    return L"(function(){"
           L"var s=document.createElement('style');"
           L"s.textContent=`" + escaped + L"`;"
           L"var t=setInterval(function(){"
           L"  if(document.head){"
           L"    document.head.appendChild(s);"
           L"    clearInterval(t);"
           L"  }"
           L"},10);"
           L"})();";
}

} // namespace webview
