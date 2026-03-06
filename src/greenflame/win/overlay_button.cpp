// Overlay toolbar button: round labelled button with hover, active, and pressed states.

#include "win/overlay_button.h"

namespace greenflame {

namespace {

constexpr BYTE kOpaqueAlpha = 255;
constexpr Gdiplus::REAL kPenWidth = 1.5f;
constexpr Gdiplus::REAL kHoverRingWidth = 3.0f;
constexpr Gdiplus::REAL kFontSize = 12.0f;

// GDI+ startup — idempotent, called at most once per process.
[[nodiscard]] bool Ensure_gdiplus() noexcept {
    static ULONG_PTR s_token = 0;
    static bool s_ok = false;
    if (!s_ok) {
        Gdiplus::GdiplusStartupInput input;
        s_ok = Gdiplus::GdiplusStartup(&s_token, &input, nullptr) == Gdiplus::Ok;
    }
    return s_ok;
}

[[nodiscard]] Gdiplus::Color To_gdip(COLORREF c, BYTE alpha = kOpaqueAlpha) noexcept {
    return Gdiplus::Color(alpha, GetRValue(c), GetGValue(c), GetBValue(c));
}

} // namespace

OverlayButton::OverlayButton(core::PointPx position, int diameter, std::wstring label,
                             bool is_toggle)
    : position_(position), diameter_(diameter), label_(std::move(label)),
      is_toggle_(is_toggle) {}

core::RectPx OverlayButton::Bounds() const {
    return core::RectPx::From_ltrb(position_.x, position_.y, position_.x + diameter_,
                                   position_.y + diameter_);
}

bool OverlayButton::Hit_test(core::PointPx pt) const {
    int const cx = position_.x + diameter_ / 2;
    int const cy = position_.y + diameter_ / 2;
    int const r = diameter_ / 2;
    int const dx = pt.x - cx;
    int const dy = pt.y - cy;
    return (dx * dx + dy * dy) <= (r * r);
}

void OverlayButton::Set_position(core::PointPx top_left) { position_ = top_left; }

void OverlayButton::On_mouse_down(core::PointPx /*pt*/) { pressed_ = true; }

void OverlayButton::On_mouse_up(core::PointPx /*pt*/) {
    pressed_ = false;
    if (is_toggle_) {
        active_ = !active_;
    }
}

void OverlayButton::Draw(HDC dc, ButtonDrawContext const &ctx) const {
    if (!dc || !Ensure_gdiplus()) {
        return;
    }

    // Render into an off-screen ARGB bitmap so GDI+ composites against a known
    // transparent background.  AlphaBlend then composites the result onto dc
    // using the destination's actual RGB values regardless of its alpha byte
    // (which GDI/GetDIBits/SetDIBits leaves as 0), giving correct source-over
    // blending against the captured screen content.
    Gdiplus::REAL const df = static_cast<Gdiplus::REAL>(diameter_);
    Gdiplus::Bitmap bmp(diameter_, diameter_, PixelFormat32bppARGB);
    {
        Gdiplus::Graphics g(&bmp);
        g.Clear(Gdiplus::Color(0, 0, 0, 0));
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

        // Three distinct visuals: normal, active (inverted), pressed (yellow-green fill
        // only).
        constexpr COLORREF k_pressed_fill = RGB(155, 220, 65);
        COLORREF fill_col, outline_col, text_col;
        if (pressed_) {
            fill_col = k_pressed_fill;
            outline_col = ctx.outline_color; // consistent regardless of active state
            text_col = ctx.outline_color;
        } else if (active_) {
            fill_col = ctx.outline_color;
            outline_col = ctx.fill_color;
            text_col = ctx.fill_color;
        } else {
            fill_col = ctx.fill_color;
            outline_col = ctx.outline_color;
            text_col = ctx.outline_color;
        }

        constexpr Gdiplus::REAL k_half_pen = kPenWidth / 2.0f;

        // Filled circle.
        {
            Gdiplus::SolidBrush fill(To_gdip(fill_col));
            g.FillEllipse(&fill, 0.0f, 0.0f, df, df);
        }

        // Outline ring.
        {
            Gdiplus::Pen outline(To_gdip(outline_col), kPenWidth);
            g.DrawEllipse(&outline, k_half_pen, k_half_pen, df - kPenWidth,
                          df - kPenWidth);
        }

        // Hover: 3px ring at the button edge, 75% outline + 25% fill color.
        if (hovered_) {
            COLORREF const hover_col =
                RGB((GetRValue(outline_col) * 3 + GetRValue(fill_col)) / 4,
                    (GetGValue(outline_col) * 3 + GetGValue(fill_col)) / 4,
                    (GetBValue(outline_col) * 3 + GetBValue(fill_col)) / 4);
            constexpr Gdiplus::REAL k_ring_double = 2.0f;
            constexpr Gdiplus::REAL k_hover_inset = kHoverRingWidth / 2.0f;
            Gdiplus::Pen ring(To_gdip(hover_col), kHoverRingWidth);
            g.DrawEllipse(&ring, k_hover_inset, k_hover_inset,
                          df - k_ring_double * k_hover_inset,
                          df - k_ring_double * k_hover_inset);
        }

        // Label with antialiased GDI+ text (no ClearType fringing).
        {
            g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
            Gdiplus::Font font(L"Segoe UI", kFontSize, Gdiplus::FontStyleBold,
                               Gdiplus::UnitPixel);
            Gdiplus::SolidBrush text_brush(To_gdip(text_col));
            Gdiplus::StringFormat fmt;
            fmt.SetAlignment(Gdiplus::StringAlignmentCenter);
            fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            Gdiplus::RectF const text_rect(0.0f, 0.0f, df, df);
            g.DrawString(label_.c_str(), static_cast<INT>(label_.size()), &font,
                         text_rect, &fmt, &text_brush);
        }
    }

    // GetHBITMAP with a transparent background premultiplies each pixel's RGB
    // by its alpha, producing the PARGB format that AlphaBlend expects.
    HBITMAP hbm = nullptr;
    if (bmp.GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hbm) == Gdiplus::Ok && hbm) {
        HDC mem_dc = CreateCompatibleDC(dc);
        if (mem_dc) {
            HGDIOBJ const old = SelectObject(mem_dc, hbm);
            BLENDFUNCTION const bf{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
            AlphaBlend(dc, position_.x, position_.y, diameter_, diameter_, mem_dc, 0, 0,
                       diameter_, diameter_, bf);
            SelectObject(mem_dc, old);
            DeleteDC(mem_dc);
        }
        DeleteObject(hbm);
    }
}

} // namespace greenflame
