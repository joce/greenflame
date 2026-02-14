#include "win/config.h"

#include <ShlObj.h>
#include <easyjson.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace greenflame {

namespace {

std::filesystem::path GetConfigPath() {
    wchar_t appData[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData)))
        return {};
    std::filesystem::path path(appData);
    path /= L"greenflame";
    path /= L"greenflame.json";
    return path;
}

}  // namespace

Config LoadConfig() {
    Config config;
    std::filesystem::path const path = GetConfigPath();
    if (path.empty() || !std::filesystem::exists(path))
        return config;
    try {
        easyjson::JSON root =
            easyjson::JSON::load_file(path.string());
        if (!root.has_key("ui"))
            return config;
        easyjson::JSON const& ui = root["ui"];
        if (!ui.has_key("show_balloons"))
            return config;
        easyjson::JSON const& val = ui["show_balloons"];
        if (val.JSON_type() == easyjson::JSON::Class::Boolean)
            config.show_balloons = val.to_bool();
    } catch (...) {
        // Parse error or file read error: return default config
    }
    return config;
}

bool SaveConfig(Config const& config) {
    std::filesystem::path const path = GetConfigPath();
    if (path.empty())
        return false;
    try {
        std::filesystem::create_directories(path.parent_path());
        easyjson::JSON root = easyjson::object();
        root["ui"]["show_balloons"] = config.show_balloons;
        std::string const json = root.dump();
        std::ofstream f(path);
        if (!f)
            return false;
        f << json;
        return f.good();
    } catch (...) {
        return false;
    }
}

}  // namespace greenflame
