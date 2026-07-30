#pragma once
// ============================================================
// ProcessManager.hpp — プロセス・スレッド優先度制御
// ============================================================
// 仕様書 §5-3 に基づく実装
//
// 競技プリセット: REALTIME_PRIORITY_CLASS + THREAD_PRIORITY_TIME_CRITICAL
// 標準/Low:      HIGH_PRIORITY_CLASS     + THREAD_PRIORITY_HIGHEST
//
// ⚠ 注意: REALTIME優先度は管理者権限が必要な場合があります。
//         他プロセスへの大きな影響があるため、ユーザーへ事前説明を表示してください。
// ============================================================

#include <windows.h>
#include <string>

namespace process {

// ============================================================
// 優先度レベル
// ============================================================
enum class PriorityLevel {
    Normal,       // NORMAL_PRIORITY_CLASS
    High,         // HIGH_PRIORITY_CLASS
    Realtime,     // REALTIME_PRIORITY_CLASS (管理者権限推奨)
};

// ============================================================
// プロセス管理クラス
// ============================================================
class ProcessManager {
public:
    static ProcessManager& Instance();

    // プロセス優先度を設定
    // 戻り値: true=成功, false=権限不足等
    bool SetProcessPriority(PriorityLevel level);
    
    // 全ての子WebView2プロセスの優先度をHIGHに引き上げる
    void ElevateChildProcesses();

    // 指定スレッドの優先度を設定する
    // @param threadPriority Win32 スレッド優先度 (例: THREAD_PRIORITY_NORMAL)
    bool SetGameThreadPriority(int threadPriority);

    // CPU アフィニティ設定 (0=自動)
    // @param mask  ビットマスク (例: 0b00001111 = コア0-3)
    bool SetCpuAffinity(DWORD_PTR mask);

    // 現在の優先度レベルを返す
    PriorityLevel GetCurrentPriority() const { return m_currentPriority; }

    // REALTIME優先度を使用するか確認するダイアログを表示
    // 戻り値: true=ユーザーが同意, false=拒否
    bool ConfirmRealtimePriority(HWND parentHwnd);

    // コピー禁止
    ProcessManager(const ProcessManager&)            = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;

private:
    ProcessManager() = default;
    PriorityLevel m_currentPriority = PriorityLevel::Normal;
    HANDLE        m_gameThread      = nullptr;
};

} // namespace process
