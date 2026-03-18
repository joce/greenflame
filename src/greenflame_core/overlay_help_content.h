#pragma once

namespace greenflame::core {

struct OverlayHelpEntry final {
    std::wstring shortcut = {};
    std::wstring description = {};
};

struct OverlayHelpSection final {
    std::wstring title = {};
    std::vector<OverlayHelpEntry> entries = {};
    bool new_column = false;
    bool gap_before = false;
};

struct OverlayHelpContent final {
    std::wstring title = {};
    std::wstring close_hint = {};
    std::vector<OverlayHelpSection> sections = {};
};

} // namespace greenflame::core
