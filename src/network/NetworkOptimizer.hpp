#pragma once
// ============================================================
// NetworkOptimizer.hpp — TCP/IPスタック最適化・レジストリ設定
// ============================================================
// 仕様書 §6-1 に基づく実装
//
// 適用する設定:
//   TcpAckFrequency = 1  → ACK遅延の無効化 (即時ACK送信)
//   TCPNoDelay      = 1  → Nagleアルゴリズムの無効化
//   TCP Window Size      → 最大化
//
// ⚠ レジストリ書き込みには管理者権限が必要です。
//   権限不足の場合はスキップし、手動設定手順を案内します。
// ============================================================

#include <windows.h>
#include <string>
#include <vector>

namespace network {

// ============================================================
// 最適化結果
// ============================================================
struct OptimizationResult {
    bool success         = false;
    bool adminRequired   = false;   // 管理者権限が必要だった
    std::string message;
};

// ============================================================
// NIC情報
// ============================================================
struct NicInfo {
    std::wstring guid;         // {XXXXXXXX-XXXX-...}
    std::wstring description;
    bool         isPhysical;
};

// ============================================================
// ネットワーク最適化クラス
// ============================================================
class NetworkOptimizer {
public:
    static NetworkOptimizer& Instance();

    // 利用可能なNICリストを取得
    std::vector<NicInfo> EnumerateNics() const;

    // TCPレジストリ最適化を適用
    // - applyGlobal: グローバルTCPパラメータ (Window Size等)
    // - applyPerNic: NICごとの設定 (TcpAckFrequency, TCPNoDelay)
    OptimizationResult ApplyTcpOptimizations(bool applyGlobal, bool applyPerNic);

    // QoSポリシー設定 (Krunkerトラフィックを優先)
    OptimizationResult ApplyQosPolicy();

    // レジストリ設定の説明テキスト (手動設定用)
    static std::wstring GetManualSettingsGuide();

    // コピー禁止
    NetworkOptimizer(const NetworkOptimizer&)            = delete;
    NetworkOptimizer& operator=(const NetworkOptimizer&) = delete;

private:
    NetworkOptimizer() = default;

    OptimizationResult ApplyGlobalTcpSettings();
    OptimizationResult ApplyPerNicSettings(const std::vector<NicInfo>& nics);

    // レジストリ値書き込みヘルパー
    static bool WriteRegDword(HKEY hKeyRoot, const std::wstring& subKey,
                              const std::wstring& valueName, DWORD value);
};

} // namespace network
