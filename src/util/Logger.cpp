// ============================================================
// Logger.cpp — Logger 実装
// ============================================================

#include "Logger.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <windows.h>

namespace util {

Logger& Logger::Instance() {
    static Logger s_instance;
    return s_instance;
}

void Logger::Init(const std::wstring& logFilePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_file.open(logFilePath, std::ios::out | std::ios::app);
    m_initialized = m_file.is_open();

    if (m_initialized) {
        // セパレータを書いてセッション開始を示す
        m_file << "\n======================================\n";
        m_file << "  KrunkerUltraClient Session Start\n";
        m_file << "======================================\n";
        m_file.flush();
    }
}

void Logger::Log(LogLevel level, const std::string& message) {
    static constexpr const char* kLevelStr[] = {
        "DEBUG", "INFO ", "WARN ", "ERROR"
    };

    // タイムスタンプ
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    ::localtime_s(&tm, &time);

    std::ostringstream ss;
    ss << "[" << std::put_time(&tm, "%H:%M:%S") << "] "
       << "[" << kLevelStr[static_cast<int>(level)] << "] "
       << message << "\n";

    const std::string logLine = ss.str();

    std::lock_guard<std::mutex> lock(m_mutex);

    // デバッグウィンドウへ出力（VS Output ウィンドウで確認可能）
    ::OutputDebugStringA(logLine.c_str());

    if (m_initialized) {
        m_file << logLine;
        if (level >= LogLevel::Warning) {
            m_file.flush(); // 警告以上は即フラッシュ
        }
    }
}

} // namespace util
