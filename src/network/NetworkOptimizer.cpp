// ============================================================
// NetworkOptimizer.cpp — TCP/IPレジストリ最適化 実装
// ============================================================

#include "NetworkOptimizer.hpp"
#include "util/Logger.hpp"
#include "util/StringUtil.hpp"

#include <winsock2.h>
#include <iphlpapi.h>
#include <winreg.h>
#include <sstream>

#pragma comment(lib, "iphlpapi.lib")

namespace {
// レジストリパス定数
constexpr const wchar_t* kTcpipParamsKey =
    L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters";
constexpr const wchar_t* kTcpipInterfacesKey =
    L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
} // namespace

namespace network {

NetworkOptimizer& NetworkOptimizer::Instance() {
    static NetworkOptimizer s_instance;
    return s_instance;
}

// ============================================================
// レジストリ書き込みヘルパー
// ============================================================
bool NetworkOptimizer::WriteRegDword(
    HKEY hKeyRoot,
    const std::wstring& subKey,
    const std::wstring& valueName,
    DWORD value)
{
    HKEY hKey = nullptr;
    LONG result = ::RegCreateKeyExW(
        hKeyRoot,
        subKey.c_str(),
        0, nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &hKey,
        nullptr
    );

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = ::RegSetValueExW(
        hKey,
        valueName.c_str(),
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&value),
        sizeof(DWORD)
    );

    ::RegCloseKey(hKey);
    return (result == ERROR_SUCCESS);
}

// ============================================================
// NIC列挙
// ============================================================
std::vector<NicInfo> NetworkOptimizer::EnumerateNics() const {
    std::vector<NicInfo> result;

    // インターフェースキーの子キーを列挙
    HKEY hKey = nullptr;
    if (::RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            kTcpipInterfacesKey,
            0, KEY_ENUMERATE_SUB_KEYS | KEY_READ,
            &hKey) != ERROR_SUCCESS) {
        LOG_WARN("NetworkOptimizer: Cannot open Tcpip Interfaces registry key. Admin required?");
        return result;
    }

    wchar_t subKeyName[256] = {};
    DWORD   subKeyNameLen   = 256;
    DWORD   index           = 0;

    while (::RegEnumKeyExW(hKey, index++, subKeyName, &subKeyNameLen,
                           nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
        NicInfo info;
        info.guid        = subKeyName;
        info.isPhysical  = true; // 詳細判定は省略
        result.push_back(info);
        subKeyNameLen = 256;
    }

    ::RegCloseKey(hKey);
    LOG_INFO("NetworkOptimizer: Found " + std::to_string(result.size()) + " NIC(s).");
    return result;
}

// ============================================================
// グローバルTCPパラメータ適用
// ============================================================
OptimizationResult NetworkOptimizer::ApplyGlobalTcpSettings() {
    OptimizationResult res;

    // GlobalMaxTcpWindowSize = 65535
    bool ok1 = WriteRegDword(HKEY_LOCAL_MACHINE, kTcpipParamsKey,
                              L"GlobalMaxTcpWindowSize", 65535);
    // TcpWindowSize = 65535
    bool ok2 = WriteRegDword(HKEY_LOCAL_MACHINE, kTcpipParamsKey,
                              L"TcpWindowSize", 65535);

    if (!ok1 || !ok2) {
        res.success       = false;
        res.adminRequired = true;
        res.message = "Failed to write global TCP parameters. Administrator rights required.";
        LOG_WARN("NetworkOptimizer: " + res.message);
        return res;
    }

    res.success = true;
    res.message = "Global TCP parameters applied (TcpWindowSize=65535).";
    LOG_INFO("NetworkOptimizer: " + res.message);
    return res;
}

// ============================================================
// NICごとの設定適用
// ============================================================
OptimizationResult NetworkOptimizer::ApplyPerNicSettings(const std::vector<NicInfo>& nics) {
    OptimizationResult res;
    int applied = 0;

    for (const auto& nic : nics) {
        std::wstring nicKey = std::wstring(kTcpipInterfacesKey) + L"\\" + nic.guid;

        // TcpAckFrequency = 1  (ACK遅延無効化)
        bool ok1 = WriteRegDword(HKEY_LOCAL_MACHINE, nicKey, L"TcpAckFrequency", 1);
        // TCPNoDelay = 1       (Nagleアルゴリズム無効化)
        bool ok2 = WriteRegDword(HKEY_LOCAL_MACHINE, nicKey, L"TCPNoDelay", 1);

        if (ok1 && ok2) {
            ++applied;
            LOG_INFO("NetworkOptimizer: Applied to NIC " + util::WideToUtf8(nic.guid));
        } else {
            LOG_WARN("NetworkOptimizer: Failed to apply to NIC " + util::WideToUtf8(nic.guid));
        }
    }

    if (applied == 0 && !nics.empty()) {
        res.success       = false;
        res.adminRequired = true;
        res.message = "Failed to apply per-NIC settings. Administrator rights required.";
    } else {
        res.success = true;
        res.message = "Per-NIC TCP settings applied to " + std::to_string(applied) + " adapter(s).";
    }

    LOG_INFO("NetworkOptimizer: " + res.message);
    return res;
}

// ============================================================
// メイン適用エントリーポイント
// ============================================================
OptimizationResult NetworkOptimizer::ApplyTcpOptimizations(bool applyGlobal, bool applyPerNic) {
    OptimizationResult res;

    if (!applyGlobal && !applyPerNic) {
        res.success = true;
        res.message = "TCP optimization skipped (disabled in settings).";
        return res;
    }

    bool allOk = true;
    std::string msg;

    if (applyGlobal) {
        auto r = ApplyGlobalTcpSettings();
        if (!r.success) { allOk = false; res.adminRequired = true; }
        msg += r.message + " ";
    }

    if (applyPerNic) {
        auto nics = EnumerateNics();
        auto r    = ApplyPerNicSettings(nics);
        if (!r.success) { allOk = false; res.adminRequired = true; }
        msg += r.message;
    }

    res.success = allOk;
    res.message = msg;
    return res;
}

OptimizationResult NetworkOptimizer::ApplyQosPolicy() {
    // QoSポリシーはグループポリシー(gpedit.msc)またはWFP(Windows Filtering Platform)で設定。
    // 本クライアントではレジストリ経由の簡易版のみ実装 (Phase 2で拡張予定)
    OptimizationResult res;
    res.success = true;
    res.message = "QoS policy: Not implemented yet (Phase 2).";
    LOG_INFO("NetworkOptimizer: " + res.message);
    return res;
}

// ============================================================
// 手動設定ガイド
// ============================================================
std::wstring NetworkOptimizer::GetManualSettingsGuide() {
    return
        L"【手動でTCPレジストリ最適化を行う方法】\n\n"
        L"管理者権限でregedit.exeを開き、以下を設定してください:\n\n"
        L"1. NICごとの設定:\n"
        L"   HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\{NIC-GUID}\n"
        L"   TcpAckFrequency = 1 (DWORD)\n"
        L"   TCPNoDelay      = 1 (DWORD)\n\n"
        L"2. グローバル設定:\n"
        L"   HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\n"
        L"   GlobalMaxTcpWindowSize = 65535 (DWORD)\n"
        L"   TcpWindowSize          = 65535 (DWORD)\n\n"
        L"設定後はWindowsを再起動してください。";
}

} // namespace network
