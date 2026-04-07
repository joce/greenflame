#include "greenflame/win/debug_log.h"
#include "greenflame/win/win32_services.h"

namespace greenflame {

Win32SpellCheckService::Win32SpellCheckService(
    std::span<const std::wstring> language_tags) {
    if (language_tags.empty()) {
        return;
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (hr == RPC_E_CHANGED_MODE) {
        // Already initialized in a different apartment mode — usable, we don't own it.
    } else if (FAILED(hr)) {
        GREENFLAME_LOG_WRITE(L"spell", L"CoInitializeEx failed");
        return;
    } else {
        co_init_owned_ = true;
    }

    Microsoft::WRL::ComPtr<ISpellCheckerFactory> factory;
    hr = CoCreateInstance(__uuidof(SpellCheckerFactory), nullptr, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&factory));
    GREENFLAME_LOG_WRITE(L"spell", std::wstring(L"CoCreateInstance hr=") +
                                       std::to_wstring(static_cast<uint32_t>(hr)) +
                                       L" factory=" + (factory ? L"ok" : L"null"));
    if (FAILED(hr) || !factory) {
        return;
    }

    for (std::wstring const &tag : language_tags) {
        BOOL supported = FALSE;
        hr = factory->IsSupported(tag.c_str(), &supported);
        GREENFLAME_LOG_WRITE(
            L"spell", std::wstring(L"IsSupported('") + tag + L"') hr=" +
                          std::to_wstring(static_cast<uint32_t>(hr)) + L" supported=" +
                          std::to_wstring(static_cast<int>(supported)));
        if (FAILED(hr) || !supported) {
            unsupported_languages_.push_back(tag);
            continue;
        }

        Microsoft::WRL::ComPtr<ISpellChecker> checker;
        hr = factory->CreateSpellChecker(tag.c_str(), &checker);
        GREENFLAME_LOG_WRITE(L"spell", std::wstring(L"CreateSpellChecker('") + tag +
                                           L"') hr=" +
                                           std::to_wstring(static_cast<uint32_t>(hr)) +
                                           L" checker=" + (checker ? L"ok" : L"null"));
        if (SUCCEEDED(hr) && checker) {
            spell_checkers_.push_back(std::move(checker));
        }
    }
}

Win32SpellCheckService::~Win32SpellCheckService() {
    spell_checkers_.clear();
    if (co_init_owned_) {
        CoUninitialize();
    }
}

namespace {

[[nodiscard]] std::vector<core::SpellError>
Check_one(Microsoft::WRL::ComPtr<ISpellChecker> const &checker,
          std::wstring const &text) {
    Microsoft::WRL::ComPtr<IEnumSpellingError> error_enum;
    HRESULT const hr = checker->Check(text.c_str(), &error_enum);
    if (FAILED(hr) || !error_enum) {
        return {};
    }

    std::vector<core::SpellError> errors;
    Microsoft::WRL::ComPtr<ISpellingError> spelling_error;
    while (error_enum->Next(&spelling_error) == S_OK) {
        CORRECTIVE_ACTION action = CORRECTIVE_ACTION_NONE;
        if (SUCCEEDED(spelling_error->get_CorrectiveAction(&action)) &&
            action != CORRECTIVE_ACTION_NONE) {
            ULONG start = 0;
            ULONG length = 0;
            if (SUCCEEDED(spelling_error->get_StartIndex(&start)) &&
                SUCCEEDED(spelling_error->get_Length(&length)) && length > 0) {
                errors.push_back(core::SpellError{static_cast<int32_t>(start),
                                                  static_cast<int32_t>(length)});
            }
        }
        spelling_error.Reset();
    }
    return errors;
}

} // namespace

std::vector<core::SpellError>
Win32SpellCheckService::Check(std::wstring_view text) const {
    if (spell_checkers_.empty() || text.empty()) {
        return {};
    }

    std::wstring const text_str(text);

    // Start with errors from the first checker.
    std::vector<core::SpellError> result = Check_one(spell_checkers_[0], text_str);

    // Intersect with each additional checker: keep only errors that every
    // checker agrees on (so a word correct in any language is not flagged).
    for (size_t i = 1; i < spell_checkers_.size() && !result.empty(); ++i) {
        std::vector<core::SpellError> const next =
            Check_one(spell_checkers_[i], text_str);
        std::vector<core::SpellError> intersection;
        for (core::SpellError const &e : result) {
            for (core::SpellError const &n : next) {
                if (e.start_utf16 == n.start_utf16 &&
                    e.length_utf16 == n.length_utf16) {
                    intersection.push_back(e);
                    break;
                }
            }
        }
        result = std::move(intersection);
    }

    GREENFLAME_LOG_WRITE(L"spell", std::wstring(L"Check: ") +
                                       std::to_wstring(result.size()) +
                                       L" errors in '" + text_str + L"'");
    return result;
}

} // namespace greenflame
