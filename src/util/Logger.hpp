#pragma once
// ============================================================
// Logger.hpp — スレッドセーフなログ出力ユーティリティ
// ============================================================
// - デバッグビルド: OutputDebugString + ファイル
// - リリースビルド: ファイルのみ
// ============================================================

#include <string>
#include <fstream>
#include <mutex>

namespace util {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    // シングルトン取得
    static Logger& Instance();

    // 初期化（呼び出し必須）
    void Init(const std::wstring& logFilePath);

    // ログ出力
    void Log(LogLevel level, const std::string& message);

    // ショートカット
    void Debug(const std::string& msg)   { Log(LogLevel::Debug,   msg); }
    void Info(const std::string& msg)    { Log(LogLevel::Info,    msg); }
    void Warning(const std::string& msg) { Log(LogLevel::Warning, msg); }
    void Error(const std::string& msg)   { Log(LogLevel::Error,   msg); }

    // コピー禁止
    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;

    std::ofstream m_file;
    std::mutex    m_mutex;
    bool          m_initialized = false;
};

} // namespace util

// ============================================================
// 便利マクロ
// ============================================================
#define LOG_DEBUG(msg)   util::Logger::Instance().Debug(msg)
#define LOG_INFO(msg)    util::Logger::Instance().Info(msg)
#define LOG_WARN(msg)    util::Logger::Instance().Warning(msg)
#define LOG_ERROR(msg)   util::Logger::Instance().Error(msg)
