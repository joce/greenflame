#pragma once

#include "greenflame_core/color_wheel.h"

namespace greenflame::core {

struct AppConfig final {
    static constexpr int32_t kDefaultBrushWidthPx = 2;
    static constexpr int32_t kDefaultToolSizeOverlayDurationMs = 800;

    std::wstring default_save_dir = {};
    std::wstring last_save_as_dir = {};
    std::wstring filename_pattern_region = {};
    std::wstring filename_pattern_desktop = {};
    std::wstring filename_pattern_monitor = {};
    std::wstring filename_pattern_window = {};
    std::wstring default_save_format = {}; // "png" (default), "jpg"/"jpeg", or "bmp".
    int32_t brush_width_px = kDefaultBrushWidthPx;
    AnnotationColorPalette annotation_colors = kDefaultAnnotationColorPalette;
    int32_t current_annotation_color_index = kDefaultAnnotationColorIndex;
    HighlighterColorPalette highlighter_colors = kDefaultHighlighterColorPalette;
    int32_t current_highlighter_color_index = kDefaultHighlighterColorIndex;
    int32_t highlighter_opacity_percent = kDefaultHighlighterOpacityPercent;
    int32_t tool_size_overlay_duration_ms = kDefaultToolSizeOverlayDurationMs;
    bool show_balloons = true;
    bool show_selection_size_side_labels = true;
    bool show_selection_size_center_label = true;

    void Normalize();
};

} // namespace greenflame::core
