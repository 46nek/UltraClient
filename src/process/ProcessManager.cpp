// ============================================================
// ProcessManager.cpp — プロセス・スレッド優先度制御 実装
// ============================================================

#include "ProcessManager.hpp"
#include "util/Logger.hpp"
#include <windows.h>
#include <tlhelp32.h>
#include <string>

namespace process {

ProcessManager& ProcessManager::Instance() {
    static ProcessManager s_instance;
    return s_instance;
}

bool ProcessManager::SetProcessPriority(PriorityLevel level) {
    DWORD priorityClass = NORMAL_PRIORITY_CLASS;
    const char* levelName = "NORMAL";

    switch (level) {
        case PriorityLevel::High:
            priorityClass = HIGH_PRIORITY_CLASS;
            levelName = "HIGH";
            break;
        case PriorityLevel::Realtime:
            priorityClass = REALTIME_PRIORITY_CLASS;
            levelName = "REALTIME";
            break;
        default:
            break;
    }

    if (!::SetPriorityClass(::GetCurrentProcess(), priorityClass)) {
        DWORD err = ::GetLastError();
        LOG_WARN("ProcessManager: SetPriorityClass failed. Error=" + std::to_string(err) +
                 ". Try running as Administrator for REALTIME priority.");
        return false;
    }

    m_currentPriority = level;
    LOG_INFO("ProcessManager: Process priority set to " + std::string(levelName));
    return true;
}



void ProcessManager::ElevateChildProcesses() {
    HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (::Process32FirstW(hSnap, &pe)) {
        do {
            std::wstring exeName = pe.szExeFile;
            for (auto& c : exeName) c = towlower(c);
            if (exeName == L"msedgewebview2.exe") {
                HANDLE hProcess = ::OpenProcess(PROCESS_SET_INFORMATION, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    ::SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
                    ::CloseHandle(hProcess);
                }
            }
        } while (::Process32NextW(hSnap, &pe));
    }
    ::CloseHandle(hSnap);
    LOG_INFO("ProcessManager: Elevated WebView2 child processes to HIGH priority.");
}

bool ProcessManager::SetGameThreadPriority(int threadPriority) {
    // 現在のスレッドをゲームスレッドとして設定
    m_gameThread = ::GetCurrentThread();

    if (!::SetThreadPriority(m_gameThread, threadPriority)) {
        DWORD err = ::GetLastError();
        LOG_WARN("ProcessManager: SetThreadPriority failed. Error=" + std::to_string(err));
        return false;
    }

    LOG_INFO("ProcessManager: Thread priority set to " + std::to_string(threadPriority));
    return true;
}

bool ProcessManager::SetCpuAffinity(DWORD_PTR mask) {
    if (mask == 0) {
        LOG_INFO("ProcessManager: CPU affinity = AUTO (skipped)");
        return true; // 0=自動、何もしない
    }

    if (!::SetProcessAffinityMask(::GetCurrentProcess(), mask)) {
        DWORD err = ::GetLastError();
        LOG_WARN("ProcessManager: SetProcessAffinityMask failed. Error=" + std::to_string(err));
        return false;
    }

    LOG_INFO("ProcessManager: CPU affinity mask set to 0x" +
             [mask]() {
                 char buf[32];
                 snprintf(buf, sizeof(buf), "%llX", (unsigned long long)mask);
                 return std::string(buf);
             }());
    return true;
}

bool ProcessManager::ConfirmRealtimePriority(HWND parentHwnd) {
    // REALTIME優先度の影響をユーザーに説明するダイアログ
    const wchar_t* msg =
        L"⚡ 競技モード: REALTIME優先度を使用します\n\n"
        L"この設定により:\n"
        L"・OSスケジューラからゲームに最高優先権を付与します\n"
        L"・他のアプリケーション（ブラウザ、音楽プレイヤー等）が\n"
        L"  一時的に応答しなくなる場合があります\n"
        L"・管理者権限が必要です\n\n"
        L"続行しますか？";

    int result = ::MessageBoxW(
        parentHwnd,
        msg,
        L"Krunker Ultra Client — 競技モード",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2
    );

    return (result == IDYES);
}

} // namespace process
