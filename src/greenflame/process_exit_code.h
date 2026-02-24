#pragma once

namespace greenflame {

enum class ProcessExitCode : int {
    Success = 0,
    WindowClassRegistrationFailed = 1,
    CliArgumentParseFailed = 2,
    TrayWindowCreateFailed = 3,
    TraySingleInstanceEnforcementFailed = 4,
    CliRegionMissing = 5,
    CliWindowNotFound = 6,
    CliWindowAmbiguous = 7,
    CliNoMonitorsAvailable = 8,
    CliMonitorOutOfRange = 9,
    CliOutputPathFailure = 10,
    CliCaptureSaveFailed = 11,
    CliWindowUnavailable = 12,
    CliWindowMinimized = 13,
};

[[nodiscard]] constexpr int To_exit_code(ProcessExitCode code) noexcept {
    return static_cast<int>(code);
}

} // namespace greenflame
