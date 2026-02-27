#include "greenflame_core/window_filter.h"

#include "greenflame_core/string_utils.h"

namespace greenflame::core {

bool Is_terminal_window_class(std::wstring_view class_name) noexcept {
    return Equals_no_case(class_name, L"ConsoleWindowClass") ||
           Equals_no_case(class_name, L"CASCADIA_HOSTING_WINDOW_CLASS");
}

bool Is_cli_invocation_window(WindowCandidateInfo const &candidate,
                              std::wstring_view query) noexcept {
    if (!Contains_no_case(candidate.title, L"greenflame.exe")) {
        return false;
    }
    if (!Contains_no_case(candidate.title, query)) {
        return false;
    }

    if (Is_terminal_window_class(candidate.class_name)) {
        return true;
    }

    return Contains_no_case(candidate.title, L"--window") ||
           Contains_no_case(candidate.title, L"-w ");
}

std::vector<WindowCandidateInfo>
Filter_cli_invocation_window(std::vector<WindowCandidateInfo> matches,
                             std::wstring_view query) {
    matches.erase(std::remove_if(matches.begin(), matches.end(),
                                 [&](WindowCandidateInfo const &candidate) {
                                     return Is_cli_invocation_window(candidate, query);
                                 }),
                  matches.end());
    return matches;
}

std::wstring Format_window_candidate_line(WindowCandidateInfo const &candidate,
                                          size_t index) {
    RectPx const normalized = candidate.rect.Normalized();
    std::wstring line = L"  [";
    line += std::to_wstring(index + 1);
    line += L"] \"";
    line += candidate.title;
    line += L"\" (x=";
    line += std::to_wstring(normalized.left);
    line += L", y=";
    line += std::to_wstring(normalized.top);
    line += L", w=";
    line += std::to_wstring(normalized.Width());
    line += L", h=";
    line += std::to_wstring(normalized.Height());
    line += L")";
    return line;
}

} // namespace greenflame::core
