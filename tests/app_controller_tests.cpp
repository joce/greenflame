#include "greenflame_core/app_config.h"
#include "greenflame_core/app_controller.h"
#include "greenflame_core/output_path.h"

using namespace greenflame;
using namespace greenflame::core;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

namespace {

class MockDisplayQueries : public IDisplayQueries {
  public:
    MOCK_METHOD(core::PointPx, Get_cursor_pos_px, (), (const, override));
    MOCK_METHOD(core::RectPx, Get_virtual_desktop_bounds_px, (), (const, override));
    MOCK_METHOD(std::vector<core::MonitorWithBounds>, Get_monitors_with_bounds, (),
                (const, override));
};

class MockWindowInspector : public IWindowInspector {
  public:
    MOCK_METHOD(std::optional<core::RectPx>, Get_window_rect, (HWND),
                (const, override));
    MOCK_METHOD(bool, Is_window_valid, (HWND), (const, override));
    MOCK_METHOD(bool, Is_window_minimized, (HWND), (const, override));
    MOCK_METHOD(WindowObscuration, Get_window_obscuration, (HWND), (const, override));
    MOCK_METHOD(std::optional<core::RectPx>, Get_foreground_window_rect, (HWND),
                (const, override));
    MOCK_METHOD(std::optional<core::RectPx>, Get_window_rect_under_cursor,
                (POINT, HWND), (const, override));
    MOCK_METHOD(std::vector<WindowMatch>, Find_windows_by_title, (std::wstring_view),
                (const, override));
};

class MockCaptureService : public ICaptureService {
  public:
    MOCK_METHOD(bool, Copy_rect_to_clipboard, (core::RectPx), (override));
    MOCK_METHOD(bool, Save_rect_to_file,
                (core::RectPx, std::wstring_view, core::ImageSaveFormat), (override));
};

class MockFileSystemService : public IFileSystemService {
  public:
    MOCK_METHOD(std::vector<std::wstring>, List_directory_filenames,
                (std::wstring_view), (const, override));
    MOCK_METHOD(std::wstring, Reserve_unique_file_path, (std::wstring_view),
                (const, override));
    MOCK_METHOD(bool, Try_reserve_exact_file_path, (std::wstring_view, bool &),
                (const, override));
    MOCK_METHOD(std::wstring, Resolve_save_directory, (std::wstring const &),
                (const, override));
    MOCK_METHOD(std::wstring, Resolve_absolute_path, (std::wstring_view),
                (const, override));
    MOCK_METHOD(void, Delete_file_if_exists, (std::wstring_view), (const, override));
    MOCK_METHOD(core::SaveTimestamp, Get_current_timestamp, (), (const, override));
};

struct ControllerFixture {
    AppConfig config = {};
    StrictMock<MockDisplayQueries> display = {};
    StrictMock<MockWindowInspector> windows = {};
    StrictMock<MockCaptureService> capture = {};
    StrictMock<MockFileSystemService> file_system = {};
    AppController controller;

    ControllerFixture() : controller(config, display, windows, capture, file_system) {}
};

} // namespace

TEST(app_controller, copy_window_uses_target_window_rect_and_copies) {
    ControllerFixture fixture;
    HWND const hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(0x1111));
    RectPx const rect = RectPx::From_ltrb(10, 20, 310, 220);

    EXPECT_CALL(fixture.windows, Get_window_rect(hwnd)).WillOnce(Return(rect));
    EXPECT_CALL(fixture.capture, Copy_rect_to_clipboard(rect)).WillOnce(Return(true));

    ClipboardCopyResult const result =
        fixture.controller.On_copy_window_to_clipboard_requested(hwnd);
    EXPECT_TRUE(result.success);
    EXPECT_THAT(result.balloon_message, HasSubstr(L"copied"));
}

TEST(app_controller, copy_last_region_without_state_returns_warning) {
    ControllerFixture fixture;

    ClipboardCopyResult const result =
        fixture.controller.On_copy_last_region_to_clipboard_requested();
    EXPECT_FALSE(result.success);
    EXPECT_THAT(result.balloon_message, HasSubstr(L"No previously captured region"));
}

TEST(app_controller, copy_last_window_reports_minimized) {
    ControllerFixture fixture;
    HWND const hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(0x2222));
    RectPx const saved = RectPx::From_ltrb(5, 6, 105, 106);
    std::ignore = fixture.controller.On_selection_copied_to_clipboard(saved, hwnd);

    EXPECT_CALL(fixture.windows, Is_window_valid(hwnd)).WillOnce(Return(true));
    EXPECT_CALL(fixture.windows, Is_window_minimized(hwnd)).WillOnce(Return(true));

    ClipboardCopyResult const result =
        fixture.controller.On_copy_last_window_to_clipboard_requested();
    EXPECT_FALSE(result.success);
    EXPECT_THAT(result.balloon_message, HasSubstr(L"minimized"));
}

TEST(app_controller, copy_last_window_requeries_and_copies) {
    ControllerFixture fixture;
    HWND const hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(0x3333));
    RectPx const saved = RectPx::From_ltrb(1, 2, 51, 52);
    RectPx const current = RectPx::From_ltrb(10, 20, 210, 220);
    std::ignore = fixture.controller.On_selection_copied_to_clipboard(saved, hwnd);

    EXPECT_CALL(fixture.windows, Is_window_valid(hwnd)).WillOnce(Return(true));
    EXPECT_CALL(fixture.windows, Is_window_minimized(hwnd)).WillOnce(Return(false));
    EXPECT_CALL(fixture.windows, Get_window_rect(hwnd)).WillOnce(Return(current));
    EXPECT_CALL(fixture.capture, Copy_rect_to_clipboard(current))
        .WillOnce(Return(true));

    ClipboardCopyResult const result =
        fixture.controller.On_copy_last_window_to_clipboard_requested();
    EXPECT_TRUE(result.success);
}

TEST(app_controller, copy_monitor_uses_cursor_monitor_bounds) {
    ControllerFixture fixture;
    MonitorWithBounds monitor0{};
    monitor0.bounds = RectPx::From_ltrb(0, 0, 100, 100);
    MonitorWithBounds monitor1{};
    monitor1.bounds = RectPx::From_ltrb(100, 0, 300, 200);
    std::vector<MonitorWithBounds> const monitors = {monitor0, monitor1};

    EXPECT_CALL(fixture.display, Get_monitors_with_bounds()).WillOnce(Return(monitors));
    EXPECT_CALL(fixture.display, Get_cursor_pos_px())
        .WillOnce(Return(PointPx{150, 50}));
    EXPECT_CALL(fixture.capture, Copy_rect_to_clipboard(monitor1.bounds))
        .WillOnce(Return(true));

    ClipboardCopyResult const result =
        fixture.controller.On_copy_monitor_to_clipboard_requested();
    EXPECT_TRUE(result.success);
}

TEST(app_controller, copy_desktop_uses_virtual_desktop_bounds) {
    ControllerFixture fixture;
    RectPx const desktop = RectPx::From_ltrb(-100, 0, 900, 700);

    EXPECT_CALL(fixture.display, Get_virtual_desktop_bounds_px())
        .WillOnce(Return(desktop));
    EXPECT_CALL(fixture.capture, Copy_rect_to_clipboard(desktop))
        .WillOnce(Return(true));

    ClipboardCopyResult const result =
        fixture.controller.On_copy_desktop_to_clipboard_requested();
    EXPECT_TRUE(result.success);
}

TEST(app_controller, on_selection_saved_builds_balloon_message_and_stores_state) {
    ControllerFixture fixture;
    HWND const hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(0x4444));
    RectPx const rect = RectPx::From_ltrb(0, 0, 100, 100);

    SelectionSavedResult const saved = fixture.controller.On_selection_saved_to_file(
        rect, hwnd, L"C:\\shots\\saved.png", true);
    EXPECT_THAT(saved.balloon_message, HasSubstr(L"saved.png"));

    EXPECT_CALL(fixture.windows, Is_window_valid(hwnd)).WillOnce(Return(true));
    EXPECT_CALL(fixture.windows, Is_window_minimized(hwnd)).WillOnce(Return(false));
    EXPECT_CALL(fixture.windows, Get_window_rect(hwnd)).WillOnce(Return(rect));
    EXPECT_CALL(fixture.capture, Copy_rect_to_clipboard(rect)).WillOnce(Return(true));

    ClipboardCopyResult const copied =
        fixture.controller.On_copy_last_window_to_clipboard_requested();
    EXPECT_TRUE(copied.success);
}

TEST(app_controller, cli_window_mode_filters_invocation_window_and_saves) {
    ControllerFixture fixture;
    CliOptions options{};
    options.capture_mode = CliCaptureMode::Window;
    options.window_name = L"Notepad";
    options.output_path = L"C:\\shots\\note.png";
    options.overwrite_output = true;

    HWND const target_hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(0x5151));
    RectPx const target_rect = RectPx::From_ltrb(10, 20, 210, 220);

    WindowMatch invocation{};
    invocation.hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(0x6161));
    invocation.info.title = L"greenflame.exe --window Notepad";
    invocation.info.class_name = L"ConsoleWindowClass";
    invocation.info.rect = RectPx::From_ltrb(0, 0, 10, 10);

    WindowMatch target{};
    target.hwnd = target_hwnd;
    target.info.title = L"Notepad";
    target.info.class_name = L"Notepad";
    target.info.rect = target_rect;

    RectPx const virtual_bounds = RectPx::From_ltrb(0, 0, 1920, 1080);
    EXPECT_CALL(fixture.windows,
                Find_windows_by_title(Eq(std::wstring_view{L"Notepad"})))
        .WillOnce(Return(std::vector<WindowMatch>{invocation, target}));
    EXPECT_CALL(fixture.windows, Is_window_valid(target_hwnd)).WillOnce(Return(true));
    EXPECT_CALL(fixture.windows, Is_window_minimized(target_hwnd))
        .WillOnce(Return(false));
    EXPECT_CALL(fixture.windows, Get_window_rect(target_hwnd))
        .WillOnce(Return(target_rect));
    EXPECT_CALL(fixture.windows, Get_window_obscuration(target_hwnd))
        .WillOnce(Return(WindowObscuration::None));
    EXPECT_CALL(fixture.display, Get_virtual_desktop_bounds_px())
        .Times(2)
        .WillRepeatedly(Return(virtual_bounds));
    EXPECT_CALL(fixture.file_system,
                Resolve_absolute_path(Eq(std::wstring_view{L"C:\\shots\\note.png"})))
        .WillOnce(Return(L"C:\\shots\\note.png"));
    EXPECT_CALL(fixture.capture,
                Save_rect_to_file(target_rect,
                                  Eq(std::wstring_view{L"C:\\shots\\note.png"}),
                                  ImageSaveFormat::Png))
        .WillOnce(Return(true));

    CliResult const result = fixture.controller.Run_cli_capture_mode(options);
    EXPECT_EQ(result.exit_code, ProcessExitCode::Success);
    EXPECT_THAT(result.stdout_message, HasSubstr(L"Saved: C:\\shots\\note.png"));
    EXPECT_TRUE(result.stderr_message.empty());
}

TEST(app_controller, cli_output_path_detects_format_conflict) {
    ControllerFixture fixture;
    CliOptions options{};
    options.capture_mode = CliCaptureMode::Desktop;
    options.output_path = L"C:\\shots\\capture.png";
    options.output_format = CliOutputFormat::Jpeg;
    options.overwrite_output = true;

    RectPx const desktop = RectPx::From_ltrb(0, 0, 1920, 1080);
    EXPECT_CALL(fixture.display, Get_virtual_desktop_bounds_px())
        .WillOnce(Return(desktop));

    CliResult const result = fixture.controller.Run_cli_capture_mode(options);
    EXPECT_EQ(result.exit_code, ProcessExitCode::CliOutputPathFailure);
    EXPECT_THAT(result.stderr_message, HasSubstr(L"conflicts with --format"));
}

TEST(app_controller, cli_default_output_path_uses_timestamp_and_num_scan) {
    ControllerFixture fixture;
    fixture.config.default_save_dir = L"C:\\shots";
    fixture.config.filename_pattern_desktop = L"shot_${num}";

    CliOptions options{};
    options.capture_mode = CliCaptureMode::Desktop;

    RectPx const desktop = RectPx::From_ltrb(0, 0, 1920, 1080);
    SaveTimestamp const timestamp{27, 2, 2026, 10, 11, 12};
    std::wstring const expected_unreserved = L"C:\\shots\\shot_000002.png";

    EXPECT_CALL(fixture.display, Get_virtual_desktop_bounds_px())
        .Times(2)
        .WillRepeatedly(Return(desktop));
    EXPECT_CALL(fixture.file_system,
                Resolve_save_directory(Eq(fixture.config.default_save_dir)))
        .WillOnce(Return(L"C:\\shots"));
    EXPECT_CALL(fixture.file_system, Get_current_timestamp())
        .WillOnce(Return(timestamp));
    EXPECT_CALL(fixture.file_system,
                List_directory_filenames(Eq(std::wstring_view{L"C:\\shots"})))
        .WillOnce(Return(std::vector<std::wstring>{L"shot_000001.png"}));
    EXPECT_CALL(fixture.file_system,
                Resolve_absolute_path(Eq(std::wstring_view{expected_unreserved})))
        .WillOnce(Return(expected_unreserved));
    EXPECT_CALL(fixture.file_system,
                Reserve_unique_file_path(Eq(std::wstring_view{expected_unreserved})))
        .WillOnce(Return(expected_unreserved));
    EXPECT_CALL(fixture.capture,
                Save_rect_to_file(desktop, Eq(std::wstring_view{expected_unreserved}),
                                  ImageSaveFormat::Png))
        .WillOnce(Return(true));

    CliResult const result = fixture.controller.Run_cli_capture_mode(options);
    EXPECT_EQ(result.exit_code, ProcessExitCode::Success);
    EXPECT_THAT(result.stdout_message, HasSubstr(expected_unreserved));
}

TEST(app_controller, cli_monitor_mode_reports_out_of_range) {
    ControllerFixture fixture;
    CliOptions options{};
    options.capture_mode = CliCaptureMode::Monitor;
    options.monitor_id = 2;

    MonitorWithBounds monitor{};
    monitor.bounds = RectPx::From_ltrb(0, 0, 100, 100);
    EXPECT_CALL(fixture.display, Get_monitors_with_bounds())
        .WillOnce(Return(std::vector<MonitorWithBounds>{monitor}));

    CliResult const result = fixture.controller.Run_cli_capture_mode(options);
    EXPECT_EQ(result.exit_code, ProcessExitCode::CliMonitorOutOfRange);
}

TEST(app_controller, cli_desktop_mode_saves_virtual_bounds) {
    ControllerFixture fixture;
    CliOptions options{};
    options.capture_mode = CliCaptureMode::Desktop;
    options.output_path = L"C:\\shots\\desktop.png";
    options.overwrite_output = true;

    RectPx const desktop = RectPx::From_ltrb(-100, 0, 1820, 1080);
    EXPECT_CALL(fixture.display, Get_virtual_desktop_bounds_px())
        .Times(2)
        .WillRepeatedly(Return(desktop));
    EXPECT_CALL(fixture.file_system,
                Resolve_absolute_path(Eq(std::wstring_view{L"C:\\shots\\desktop.png"})))
        .WillOnce(Return(L"C:\\shots\\desktop.png"));
    EXPECT_CALL(fixture.capture,
                Save_rect_to_file(desktop,
                                  Eq(std::wstring_view{L"C:\\shots\\desktop.png"}),
                                  ImageSaveFormat::Png))
        .WillOnce(Return(true));

    CliResult const result = fixture.controller.Run_cli_capture_mode(options);
    EXPECT_EQ(result.exit_code, ProcessExitCode::Success);
}

TEST(app_controller, cli_explicit_output_without_overwrite_checks_reservation) {
    ControllerFixture fixture;
    CliOptions options{};
    options.capture_mode = CliCaptureMode::Desktop;
    options.output_path = L"C:\\shots\\exists.png";

    RectPx const desktop = RectPx::From_ltrb(0, 0, 100, 100);
    EXPECT_CALL(fixture.display, Get_virtual_desktop_bounds_px())
        .WillOnce(Return(desktop));
    EXPECT_CALL(fixture.file_system,
                Resolve_absolute_path(Eq(std::wstring_view{L"C:\\shots\\exists.png"})))
        .WillOnce(Return(L"C:\\shots\\exists.png"));
    EXPECT_CALL(
        fixture.file_system,
        Try_reserve_exact_file_path(Eq(std::wstring_view{L"C:\\shots\\exists.png"}), _))
        .WillOnce(DoAll(SetArgReferee<1>(true), Return(false)));

    CliResult const result = fixture.controller.Run_cli_capture_mode(options);
    EXPECT_EQ(result.exit_code, ProcessExitCode::CliOutputPathFailure);
    EXPECT_THAT(result.stderr_message, HasSubstr(L"already exists"));
}

TEST(app_controller,
     cli_save_failure_deletes_reserved_output_path_when_reservation_requested) {
    ControllerFixture fixture;
    CliOptions options{};
    options.capture_mode = CliCaptureMode::Desktop;
    options.output_path = L"C:\\shots\\fail.png";

    RectPx const desktop = RectPx::From_ltrb(0, 0, 100, 100);
    EXPECT_CALL(fixture.display, Get_virtual_desktop_bounds_px())
        .Times(2)
        .WillRepeatedly(Return(desktop));
    EXPECT_CALL(fixture.file_system,
                Resolve_absolute_path(Eq(std::wstring_view{L"C:\\shots\\fail.png"})))
        .WillOnce(Return(L"C:\\shots\\fail.png"));
    EXPECT_CALL(
        fixture.file_system,
        Try_reserve_exact_file_path(Eq(std::wstring_view{L"C:\\shots\\fail.png"}), _))
        .WillOnce(Return(true));
    EXPECT_CALL(fixture.capture,
                Save_rect_to_file(desktop,
                                  Eq(std::wstring_view{L"C:\\shots\\fail.png"}),
                                  ImageSaveFormat::Png))
        .WillOnce(Return(false));
    EXPECT_CALL(fixture.file_system,
                Delete_file_if_exists(Eq(std::wstring_view{L"C:\\shots\\fail.png"})));

    CliResult const result = fixture.controller.Run_cli_capture_mode(options);
    EXPECT_EQ(result.exit_code, ProcessExitCode::CliCaptureSaveFailed);
    EXPECT_THAT(result.stderr_message, HasSubstr(L"Failed to encode or write image"));
}
