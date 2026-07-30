// ============================================================
// ResourceFilter.cpp — 広告・トラッカーブロック 実装
// ============================================================

#include "ResourceFilter.hpp"
#include "util/Logger.hpp"
#include "util/StringUtil.hpp"
#include <algorithm>
#include <cctype>
#include <windows.h>

namespace webview {

ResourceFilter& ResourceFilter::Instance() {
    static ResourceFilter s_instance;
    return s_instance;
}

// ============================================================
// デフォルトルール読み込み
// ============================================================
void ResourceFilter::LoadDefaultRules() {
    m_allowedDomains.clear();
    m_blockRules.clear();

    // ============================================================
    // 許可ドメイン (ホワイトリスト)
    // Krunkerのゲームプレイに必要なドメインのみ許可
    // ============================================================
    const std::vector<std::string> allowedDomains = {
        "krunker.io",
        "krunker.com",
        "social.krunker.io",
        // WebView2/Edgeが使用するシステムドメイン
        "edge.microsoft.com",
        "login.microsoftonline.com",
        // CDN (Krunkerアセット配信)
        "cdn.krunker.io",
        "static.krunker.io",
    };

    for (const auto& d : allowedDomains) {
        m_allowedDomains.insert(d);
    }

    // ============================================================
    // ブロックパターン
    // 広告・トラッカー・不要なアナリティクスをブロック
    // ============================================================
    const std::vector<std::pair<std::string, std::string>> blockPatterns = {
        // 広告ネットワーク
        { "doubleclick.net",       "Google Ads" },
        { "googlesyndication.com", "Google Ads" },
        { "googleadservices.com",  "Google Ads" },
        { "adsystem.com",          "Ad Network" },
        { "adnxs.com",             "AppNexus Ads" },
        { "outbrain.com",          "Outbrain Ads" },
        { "taboola.com",           "Taboola Ads" },
        { "adsrvr.org",            "TradeDesk Ads" },
        { "advertising.com",       "Verizon Ads" },
        // トラッカー
        { "google-analytics.com",  "Google Analytics" },
        { "googletagmanager.com",  "Google Tag Manager" },
        { "hotjar.com",            "Hotjar" },
        { "mixpanel.com",          "Mixpanel" },
        { "segment.com",           "Segment" },
        { "amplitude.com",         "Amplitude" },
        { "facebook.com/tr",       "Facebook Pixel" },
        { "facebook.net",          "Facebook SDK" },
        { "twitter.com/i/jot",     "Twitter Analytics" },
        { "analytics.tiktok.com",  "TikTok Analytics" },
        // パフォーマンスに影響する不要リソース
        { "intercom.io",           "Intercom (chat)" },
        { "zendesk.com",           "Zendesk" },
        { "crisp.chat",            "Crisp Chat" },
        { "widget.freshworks.com", "Freshworks Widget" },
        // Krunker内の不要なUI要素 (将来的に細調整)
        // { "social.krunker.io/banner", "Krunker Banner Ads" },
    };

    for (const auto& [pattern, reason] : blockPatterns) {
        m_blockRules.push_back({ pattern, FilterAction::Block, reason });
    }

    LOG_INFO("ResourceFilter: Loaded " + std::to_string(m_allowedDomains.size()) +
             " allowed domains and " + std::to_string(m_blockRules.size()) + " block rules.");
}

// ============================================================
// URL評価
// ============================================================
void ResourceFilter::SetSwapperDirectory(const std::wstring& dir) {
    m_swapperDir = dir;
}

std::wstring ResourceFilter::GetSwapFilePath(const std::string& url) const {
    if (m_swapperDir.empty()) return L"";
    
    std::string path = url;
    size_t prefixPos = path.find("://");
    if (prefixPos != std::string::npos) {
        path = path.substr(prefixPos + 3);
    }
    
    // クエリ文字列やハッシュがあれば除去
    size_t qPos = path.find('?');
    if (qPos != std::string::npos) path = path.substr(0, qPos);
    size_t hPos = path.find('#');
    if (hPos != std::string::npos) path = path.substr(0, hPos);

    // / を \ に置換
    std::wstring wPath = util::Utf8ToWide(path);
    for (wchar_t& c : wPath) {
        if (c == L'/') c = L'\\';
    }

    std::wstring fullPath = m_swapperDir + L"\\" + wPath;
    DWORD attrs = ::GetFileAttributesW(fullPath.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return fullPath;
    }

    // Direct filename match (allow users to just drop files in swapper/)
    size_t lastSlash = wPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        std::wstring fileName = wPath.substr(lastSlash + 1);
        std::wstring directPath = m_swapperDir + L"\\" + fileName;
        attrs = ::GetFileAttributesW(directPath.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return directPath;
        }
    }
    
    return L"";
}

FilterAction ResourceFilter::Evaluate(const std::string& url) const {
    if (!m_swapperDir.empty()) {
        std::wstring localPath = GetSwapFilePath(url);
        if (!localPath.empty()) {
            return FilterAction::Swap;
        }
    }

    // ブロックルールを先に評価 (大文字小文字を区別しない簡易比較のため、
    // Krunkerでは小文字に変換したURLを使用)
    std::string lowerUrl = url;
    for (char& c : lowerUrl) {
        if (c >= 'A' && c <= 'Z') c += 32;
    }

    // ブロックルールを評価
    for (const auto& rule : m_blockRules) {
        if (lowerUrl.find(rule.pattern) != std::string::npos) {
            ++m_blockedCount;
            // ログは重いのでDEBUGのみ出力 (既にDEBUGなのでOK)
            return FilterAction::Block;
        }
    }

    ++m_allowedCount;
    return FilterAction::Allow;
}

void ResourceFilter::AddAllowedDomain(const std::string& domain) {
    m_allowedDomains.insert(domain);
}

void ResourceFilter::AddBlockPattern(const std::string& pattern, const std::string& reason) {
    m_blockRules.push_back({ pattern, FilterAction::Block, reason.empty() ? "Custom" : reason });
}

} // namespace webview
