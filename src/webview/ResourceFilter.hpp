#pragma once
// ============================================================
// ResourceFilter.hpp — 広告・トラッカーブロック
// ============================================================
// 仕様書 §6-3 に基づく実装
//
// WebView2の WebResourceRequested イベントを利用して:
// - 広告・トラッカーのリクエストをC++レベルでブロック
// - Krunkerゲームプレイに不要なAPIをインターセプト
// - ホワイトリスト型フィルタ (許可ドメイン以外をブロック)
// ============================================================

#include <string>
#include <vector>
#include <unordered_set>

namespace webview {

// ============================================================
// フィルタリング判定
// ============================================================
enum class FilterAction {
    Allow,  // 許可
    Block,  // ブロック (空レスポンスを返す)
    Swap,   // スワップ (ローカルファイルを返す)
};

// ============================================================
// フィルタルール
// ============================================================
struct FilterRule {
    std::string pattern;  // URLパターン (部分一致)
    FilterAction action;
    std::string reason;   // ログ用
};

// ============================================================
// リソースフィルタークラス
// ============================================================
class ResourceFilter {
public:
    static ResourceFilter& Instance();

    // デフォルトルールを読み込む (広告・トラッカー一覧)
    void LoadDefaultRules();

    // URLを評価してアクションを返す
    FilterAction Evaluate(const std::string& url) const;

    // 許可ドメインを追加
    void AddAllowedDomain(const std::string& domain);

    // ブロックパターンを追加
    void AddBlockPattern(const std::string& pattern, const std::string& reason = "");

    void SetSwapperDirectory(const std::wstring& dir);
    std::wstring GetSwapFilePath(const std::string& url) const;

    // 統計
    int GetBlockedCount()  const { return m_blockedCount; }
    int GetAllowedCount()  const { return m_allowedCount; }
    void ResetStats() { m_blockedCount = 0; m_allowedCount = 0; }

    // コピー禁止
    ResourceFilter(const ResourceFilter&)            = delete;
    ResourceFilter& operator=(const ResourceFilter&) = delete;

private:
    ResourceFilter() = default;

    std::vector<FilterRule>         m_blockRules;
    std::unordered_set<std::string> m_allowedDomains;
    std::wstring                    m_swapperDir;

    mutable int m_blockedCount = 0;
    mutable int m_allowedCount = 0;
};

} // namespace webview
