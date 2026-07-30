// ============================================================
// Settings.cpp — 設定管理・JSON読み書き
// ============================================================

#include "Settings.hpp"
#include "util/Logger.hpp"
#include "util/StringUtil.hpp"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/error/en.h>

#include <windows.h>
#include <fstream>
#include <cstdio>

namespace settings {

SettingsManager& SettingsManager::Instance() {
    static SettingsManager s_instance;
    return s_instance;
}

// -----------------------------------------------
// JSON → AppSettings 読み込みヘルパー
// -----------------------------------------------
static void ParseProcess(const rapidjson::Value& v, ProcessConfig& p) {
    if (v.HasMember("highPriority") && v["highPriority"].IsBool())
        p.highPriority = v["highPriority"].GetBool();
    if (v.HasMember("cpuAffinityMask") && v["cpuAffinityMask"].IsInt())
        p.cpuAffinityMask = v["cpuAffinityMask"].GetInt();
}

static void ParseBrowser(const rapidjson::Value& v, BrowserConfig& b) {
    if (v.HasMember("hardwareAccel") && v["hardwareAccel"].IsBool())
        b.hardwareAccel = v["hardwareAccel"].GetBool();
    if (v.HasMember("disableVSync") && v["disableVSync"].IsBool())
        b.disableVSync = v["disableVSync"].GetBool();
    if (v.HasMember("ignoreGpuBlocklist") && v["ignoreGpuBlocklist"].IsBool())
        b.ignoreGpuBlocklist = v["ignoreGpuBlocklist"].GetBool();
    if (v.HasMember("disableBackgroundThrottling") && v["disableBackgroundThrottling"].IsBool())
        b.disableBackgroundThrottling = v["disableBackgroundThrottling"].GetBool();
    if (v.HasMember("mouseFlickFix") && v["mouseFlickFix"].IsBool())
        b.mouseFlickFix = v["mouseFlickFix"].GetBool();
}

static void ParseNetwork(const rapidjson::Value& v, NetworkConfig& n) {
    if (v.HasMember("disableNagle") && v["disableNagle"].IsBool())
        n.disableNagle = v["disableNagle"].GetBool();
    if (v.HasMember("applyTcpRegistry") && v["applyTcpRegistry"].IsBool())
        n.applyTcpRegistry = v["applyTcpRegistry"].GetBool();
}

static void ParseExtension(const rapidjson::Value& v, ExtensionConfig& e) {
    if (v.HasMember("blockAds") && v["blockAds"].IsBool())
        e.blockAds = v["blockAds"].GetBool();
    if (v.HasMember("startFullscreen") && v["startFullscreen"].IsBool())
        e.startFullscreen = v["startFullscreen"].GetBool();
    if (v.HasMember("enableSwapper") && v["enableSwapper"].IsBool())
        e.enableSwapper = v["enableSwapper"].GetBool();
    if (v.HasMember("enableUserscripts") && v["enableUserscripts"].IsBool())
        e.enableUserscripts = v["enableUserscripts"].GetBool();
}

bool SettingsManager::Load(const std::wstring& filePath) {
    m_settings = AppSettings(); // デフォルト値リセット
    m_settings.settingsFilePath = filePath;

    FILE* fp = nullptr;
    if (_wfopen_s(&fp, filePath.c_str(), L"r") != 0 || !fp) {
        LOG_INFO("Settings: No settings.json found. Using defaults.");
        return false;
    }

    char readBuf[65536];
    rapidjson::FileReadStream is(fp, readBuf, sizeof(readBuf));
    rapidjson::Document doc;
    doc.ParseStream(is);
    fclose(fp);

    if (doc.HasParseError()) {
        LOG_WARN("Settings: JSON parse error: " +
            std::string(rapidjson::GetParseError_En(doc.GetParseError())));
        return false;
    }

    if (doc.HasMember("process") && doc["process"].IsObject())
        ParseProcess(doc["process"], m_settings.process);
    if (doc.HasMember("browser") && doc["browser"].IsObject())
        ParseBrowser(doc["browser"], m_settings.browser);
    if (doc.HasMember("network") && doc["network"].IsObject())
        ParseNetwork(doc["network"], m_settings.network);
    if (doc.HasMember("extension") && doc["extension"].IsObject())
        ParseExtension(doc["extension"], m_settings.extension);

    LOG_INFO("Settings: Loaded from " + util::WideToUtf8(filePath));
    return true;
}

bool SettingsManager::Save() const {
    if (m_settings.settingsFilePath.empty()) return false;

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> w(sb);

    w.StartObject();

    w.Key("process"); w.StartObject();
    {
        const auto& p = m_settings.process;
        w.Key("highPriority");    w.Bool(p.highPriority);
        w.Key("cpuAffinityMask"); w.Int(p.cpuAffinityMask);
    }
    w.EndObject();

    w.Key("browser"); w.StartObject();
    {
        const auto& b = m_settings.browser;
        w.Key("hardwareAccel");              w.Bool(b.hardwareAccel);
        w.Key("disableVSync");               w.Bool(b.disableVSync);
        w.Key("ignoreGpuBlocklist");         w.Bool(b.ignoreGpuBlocklist);
        w.Key("disableBackgroundThrottling"); w.Bool(b.disableBackgroundThrottling);
        w.Key("mouseFlickFix");              w.Bool(b.mouseFlickFix);
    }
    w.EndObject();

    w.Key("network"); w.StartObject();
    {
        const auto& n = m_settings.network;
        w.Key("disableNagle");     w.Bool(n.disableNagle);
        w.Key("applyTcpRegistry"); w.Bool(n.applyTcpRegistry);
    }
    w.EndObject();

    w.Key("extension"); w.StartObject();
    {
        const auto& e = m_settings.extension;
        w.Key("blockAds");        w.Bool(e.blockAds);
        w.Key("startFullscreen"); w.Bool(e.startFullscreen);
        w.Key("enableSwapper");   w.Bool(e.enableSwapper);
        w.Key("enableUserscripts"); w.Bool(e.enableUserscripts);
    }
    w.EndObject();

    w.EndObject();

    FILE* fp = nullptr;
    if (_wfopen_s(&fp, m_settings.settingsFilePath.c_str(), L"w") != 0 || !fp) {
        LOG_ERROR("Settings: Failed to open settings.json for writing.");
        return false;
    }
    fputs(sb.GetString(), fp);
    fclose(fp);

    LOG_INFO("Settings: Saved to " + util::WideToUtf8(m_settings.settingsFilePath));
    return true;
}

} // namespace settings
