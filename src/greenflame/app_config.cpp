#include "app_config.h"

#include <ShlObj.h>
#include <easyjson.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace greenflame {

namespace {

std::filesystem::path GetConfigPath() {
    wchar_t app_data[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, app_data))) {
        return {};
    }
    std::filesystem::path path(app_data);
    path /= L"greenflame";
    path /= L"greenflame.json";
    return path;
}

std::wstring ToWide(std::string const& value) {
    if (value.empty()) {
        return {};
    }
    int const required_chars =
        MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (required_chars <= 1) {
        return {};
    }
    std::wstring out(static_cast<size_t>(required_chars - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, out.data(),
                        required_chars);
    return out;
}

std::string ToUtf8(std::wstring const& value) {
    if (value.empty()) {
        return {};
    }
    int const required_chars =
        WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr,
                            nullptr);
    if (required_chars <= 1) {
        return {};
    }
    std::string out(static_cast<size_t>(required_chars - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, out.data(),
                        required_chars, nullptr, nullptr);
    return out;
}

}  // namespace

AppConfig AppConfig::Load() {
    AppConfig config;
    std::filesystem::path const path = GetConfigPath();
    if (path.empty() || !std::filesystem::exists(path)) {
        return config;
    }
    try {
        easyjson::JSON root = easyjson::JSON::load_file(path.string());
        if (root.has_key("ui")) {
            easyjson::JSON const& ui = root["ui"];
            if (ui.has_key("show_balloons")) {
                easyjson::JSON const& value = ui["show_balloons"];
                if (value.JSON_type() == easyjson::JSON::Class::Boolean) {
                    config.show_balloons = value.to_bool();
                }
            }
        }
        if (root.has_key("save")) {
            easyjson::JSON const& save = root["save"];
            if (save.has_key("last_save_dir")) {
                easyjson::JSON const& value = save["last_save_dir"];
                if (value.JSON_type() == easyjson::JSON::Class::String) {
                    config.last_save_dir = ToWide(value.to_string());
                }
            }
        }
    } catch (...) {
        // Parse error or file read error: return default config.
    }
    config.Normalize();
    return config;
}

bool AppConfig::Save() const {
    std::filesystem::path const path = GetConfigPath();
    if (path.empty()) {
        return false;
    }
    try {
        std::filesystem::create_directories(path.parent_path());
        easyjson::JSON root = easyjson::object();
        root["ui"]["show_balloons"] = show_balloons;
        root["save"]["last_save_dir"] = ToUtf8(last_save_dir);

        std::ofstream file(path);
        if (!file) {
            return false;
        }
        file << root.dump();
        return file.good();
    } catch (...) {
        return false;
    }
}

void AppConfig::Normalize() {
    if (last_save_dir.size() >= MAX_PATH) {
        last_save_dir.resize(MAX_PATH - 1);
    }
}

}  // namespace greenflame

