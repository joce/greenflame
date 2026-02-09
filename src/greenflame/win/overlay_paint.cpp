// Overlay painting: capture blit, selection dim/border, dimension labels,
// crosshair, round magnifier and coord tooltip. Implementation and internal helpers.

#include "overlay_paint.h"

#include "greenflame_core/pixel_ops.h"
#include "greenflame_core/rect_px.h"
#include "gdi_capture.h"

#include <windows.h>

#include <cstdint>
#include <vector>
#include <wchar.h>

namespace {

constexpr COLORREF kCrosshairColor =
        RGB(0x20, 0xB2, 0xAA);  // LightSeaGreen (Greenshot-style)
constexpr unsigned char kCoordTooltipAlpha = 200;
constexpr unsigned char kCoordTooltipBgR = 217, kCoordTooltipBgG = 240,
                                                kCoordTooltipBgB = 227;
constexpr COLORREF kCoordTooltipBorderText = RGB(46, 139, 87);  // SeaGreen

constexpr int kHandleHalfSize = 4;  // 8x8 handle centered on contour
constexpr int kMagnifierSize = 256;
constexpr int kMagnifierZoom = 8;  // source size = kMagnifierSize / kMagnifierZoom
constexpr int kMagnifierSource = kMagnifierSize / kMagnifierZoom;  // 64
constexpr int kMagnifierPadding = 8;

void DrawCaptureToBuffer(HDC bufDc, HDC hdc, int w, int h,
                                                  greenflame::GdiCaptureResult const* capture) {
    if (!capture || !capture->IsValid())
        return;
    HDC srcDc = CreateCompatibleDC(hdc);
    if (srcDc) {
        HGDIOBJ oldSrc = SelectObject(srcDc, capture->bitmap);
        BitBlt(bufDc, 0, 0, w, h, srcDc, 0, 0, SRCCOPY);
        SelectObject(srcDc, oldSrc);
        DeleteDC(srcDc);
    }
}

void DrawContourHandles(HDC bufDc,
                                                greenflame::core::RectPx const& sel) {
    if (sel.IsEmpty())
        return;
    greenflame::core::RectPx const r = sel.Normalized();
    int const cx = (r.left + r.right) / 2;
    int const cy = (r.top + r.bottom) / 2;
    COLORREF const handleColor = RGB(0, 0x80, 0x80);
    HBRUSH brush = CreateSolidBrush(handleColor);
    HPEN pen = CreatePen(PS_SOLID, 1, handleColor);
    if (!brush || !pen)
        return;
    HGDIOBJ oldBrush = SelectObject(bufDc, brush);
    HGDIOBJ oldPen = SelectObject(bufDc, pen);
    auto drawHandle = [bufDc](int hx, int hy) {
        RECT hr = {hx - kHandleHalfSize, hy - kHandleHalfSize,
                              hx + kHandleHalfSize, hy + kHandleHalfSize};
        Rectangle(bufDc, hr.left, hr.top, hr.right, hr.bottom);
    };
    drawHandle(r.left, r.top);
    drawHandle(r.right, r.top);
    drawHandle(r.right, r.bottom);
    drawHandle(r.left, r.bottom);
    drawHandle(cx, r.top);
    drawHandle(r.right, cy);
    drawHandle(cx, r.bottom);
    drawHandle(r.left, cy);
    SelectObject(bufDc, oldPen);
    SelectObject(bufDc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void DrawSelectionDimAndBorder(HDC bufDc, HBITMAP bufBmp, int w, int h,
                                                              greenflame::core::RectPx const& sel) {
    if (sel.IsEmpty())
        return;
    int const rowBytes = greenflame::RowBytes32(w);
    size_t const size = static_cast<size_t>(rowBytes) * static_cast<size_t>(h);
    std::vector<uint8_t> pixels(size);
    BITMAPINFOHEADER bmi;
    greenflame::FillBmi32TopDown(bmi, w, h);
    if (GetDIBits(bufDc, bufBmp, 0, static_cast<UINT>(h), pixels.data(),
                                reinterpret_cast<BITMAPINFO*>(&bmi), DIB_RGB_COLORS) != 0) {
        greenflame::core::DimPixelsOutsideRect(pixels, w, h, rowBytes, sel);
        SetDIBits(bufDc, bufBmp, 0, static_cast<UINT>(h), pixels.data(),
                            reinterpret_cast<BITMAPINFO*>(&bmi), DIB_RGB_COLORS);
    }
    HBRUSH borderBrush = CreateSolidBrush(RGB(0, 0x80, 0x80));
    if (borderBrush) {
        RECT selRc = {sel.left, sel.top, sel.right, sel.bottom};
        FrameRect(bufDc, &selRc, borderBrush);
        DeleteObject(borderBrush);
    }
}

void DrawDimensionLabels(HDC bufDc, HBITMAP bufBmp, int w, int h,
                                                  greenflame::core::RectPx const& sel) {
    constexpr int kDimMargin = 4;
    constexpr int kDimGap = 4;
    constexpr int kCenterMarginV = 2;
    int const rowBytes = greenflame::RowBytes32(w);
    size_t const pixSize =
            static_cast<size_t>(rowBytes) * static_cast<size_t>(h);
    std::vector<uint8_t> labelPixels(pixSize);
    BITMAPINFOHEADER bmiDim;
    greenflame::FillBmi32TopDown(bmiDim, w, h);
    if (GetDIBits(bufDc, bufBmp, 0, static_cast<UINT>(h), labelPixels.data(),
                                reinterpret_cast<BITMAPINFO*>(&bmiDim),
                                DIB_RGB_COLORS) != 0) {
        HFONT fontDim =
                CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      DEFAULT_QUALITY, FF_DONTCARE, L"Segoe UI");
        HFONT fontCenter =
                CreateFontW(36, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      DEFAULT_QUALITY, FF_DONTCARE, L"Segoe UI");
        if (fontDim) {
            HGDIOBJ oldFontDim = SelectObject(bufDc, fontDim);

            wchar_t widthBuf[32], heightBuf[32], centerBuf[32];
            swprintf_s(widthBuf, L"%d", sel.Width());
            swprintf_s(heightBuf, L"%d", sel.Height());
            swprintf_s(centerBuf, L"%d x %d", sel.Width(), sel.Height());

            SIZE widthSize = {}, heightSize = {}, centerSize = {};
            GetTextExtentPoint32W(bufDc, widthBuf,
                                                        static_cast<int>(wcslen(widthBuf)), &widthSize);
            GetTextExtentPoint32W(bufDc, heightBuf,
                                                        static_cast<int>(wcslen(heightBuf)), &heightSize);
            if (fontCenter) {
                SelectObject(bufDc, fontCenter);
                GetTextExtentPoint32W(bufDc, centerBuf,
                                                            static_cast<int>(wcslen(centerBuf)), &centerSize);
                SelectObject(bufDc, fontDim);
            }

            int const widthBoxW = widthSize.cx + 2 * kDimMargin;
            int const widthBoxH = widthSize.cy + 2 * kDimMargin;
            int const heightBoxW = heightSize.cx + 2 * kDimMargin;
            int const heightBoxH = heightSize.cy + 2 * kDimMargin;
            int const centerBoxW = centerSize.cx + 2 * kDimMargin;
            int const centerBoxH = centerSize.cy + 2 * kCenterMarginV;

            int const centerX = (sel.left + sel.right) / 2;
            int const centerY = (sel.top + sel.bottom) / 2;

            int widthBoxLeft = centerX - widthBoxW / 2;
            int widthBoxTop;
            bool aboveFits = (sel.top - widthBoxH - kDimGap >= 0);
            bool belowFits = (sel.bottom + kDimGap + widthBoxH <= h);
            if (aboveFits)
                widthBoxTop = sel.top - widthBoxH - kDimGap;
            else if (belowFits)
                widthBoxTop = sel.bottom + kDimGap;
            else
                widthBoxTop = (sel.top - widthBoxH - kDimGap >= h - widthBoxH)
                                                    ? (sel.bottom + kDimGap)
                                                    : (sel.top - widthBoxH - kDimGap);
            if (widthBoxTop < 0)
                widthBoxTop = 0;
            if (widthBoxTop + widthBoxH > h)
                widthBoxTop = h - widthBoxH;

            int heightBoxTop = centerY - heightBoxH / 2;
            int heightBoxLeft;
            int heightBoxRightLeft = sel.left - kDimGap - heightBoxW;
            int heightBoxRightRight = sel.right + kDimGap;
            bool leftFits = (heightBoxRightLeft >= 0);
            bool rightFits = (heightBoxRightRight + heightBoxW <= w);
            if (leftFits) {
                heightBoxLeft = heightBoxRightLeft;
            } else if (rightFits) {
                heightBoxLeft = heightBoxRightRight;
            } else {
                heightBoxLeft = (heightBoxRightLeft >= w - heightBoxW)
                                                        ? heightBoxRightRight
                                                        : heightBoxRightLeft;
            }
            if (heightBoxLeft < 0)
                heightBoxLeft = 0;
            if (heightBoxLeft + heightBoxW > w)
                heightBoxLeft = w - heightBoxW;

            int centerBoxLeft = centerX - centerBoxW / 2;
            int centerBoxTop = centerY - centerBoxH / 2;
            constexpr int kCenterMinPadding = 24;
            int const selW = sel.Width();
            int const selH = sel.Height();
            bool const centerFits =
                    (selW >= centerBoxW + 2 * kCenterMinPadding &&
                      selH >= centerBoxH + 2 * kCenterMinPadding);

            greenflame::core::RectPx widthBoxRect = greenflame::core::RectPx::FromLtrb(
                    widthBoxLeft, widthBoxTop, widthBoxLeft + widthBoxW,
                    widthBoxTop + widthBoxH);
            greenflame::core::RectPx heightBoxRect = greenflame::core::RectPx::FromLtrb(
                    heightBoxLeft, heightBoxTop, heightBoxLeft + heightBoxW,
                    heightBoxTop + heightBoxH);
            greenflame::core::RectPx centerBoxRect = greenflame::core::RectPx::FromLtrb(
                    centerBoxLeft, centerBoxTop, centerBoxLeft + centerBoxW,
                    centerBoxTop + centerBoxH);

            greenflame::core::BlendRectOntoPixels(
                    labelPixels, w, h, rowBytes, widthBoxRect, kCoordTooltipBgR,
                    kCoordTooltipBgG, kCoordTooltipBgB, kCoordTooltipAlpha);
            greenflame::core::BlendRectOntoPixels(
                    labelPixels, w, h, rowBytes, heightBoxRect, kCoordTooltipBgR,
                    kCoordTooltipBgG, kCoordTooltipBgB, kCoordTooltipAlpha);
            if (centerFits)
                greenflame::core::BlendRectOntoPixels(
                        labelPixels, w, h, rowBytes, centerBoxRect, kCoordTooltipBgR,
                        kCoordTooltipBgG, kCoordTooltipBgB, kCoordTooltipAlpha);
            SetDIBits(bufDc, bufBmp, 0, static_cast<UINT>(h), labelPixels.data(),
                                reinterpret_cast<BITMAPINFO*>(&bmiDim), DIB_RGB_COLORS);

            HPEN dimBorderPen = CreatePen(PS_SOLID, 1, kCoordTooltipBorderText);
            if (dimBorderPen) {
                HGDIOBJ oldDimPen = SelectObject(bufDc, dimBorderPen);
                SelectObject(bufDc, GetStockObject(NULL_BRUSH));
                Rectangle(bufDc, widthBoxLeft, widthBoxTop, widthBoxLeft + widthBoxW,
                                    widthBoxTop + widthBoxH);
                Rectangle(bufDc, heightBoxLeft, heightBoxTop,
                                    heightBoxLeft + heightBoxW, heightBoxTop + heightBoxH);
                if (centerFits)
                    Rectangle(bufDc, centerBoxLeft, centerBoxTop,
                                        centerBoxLeft + centerBoxW, centerBoxTop + centerBoxH);
                SelectObject(bufDc, oldDimPen);
                DeleteObject(dimBorderPen);
            }
            SetBkMode(bufDc, TRANSPARENT);
            SetTextColor(bufDc, kCoordTooltipBorderText);
            RECT widthTextRc = {widthBoxLeft + kDimMargin, widthBoxTop + kDimMargin,
                                                    widthBoxLeft + widthBoxW - kDimMargin,
                                                    widthBoxTop + widthBoxH - kDimMargin};
            RECT heightTextRc = {heightBoxLeft + kDimMargin,
                                                      heightBoxTop + kDimMargin,
                                                      heightBoxLeft + heightBoxW - kDimMargin,
                                                      heightBoxTop + heightBoxH - kDimMargin};
            RECT centerTextRc = {centerBoxLeft + kDimMargin,
                                                      centerBoxTop + kCenterMarginV,
                                                      centerBoxLeft + centerBoxW - kDimMargin,
                                                      centerBoxTop + centerBoxH - kCenterMarginV};
            DrawTextW(bufDc, widthBuf, -1, &widthTextRc,
                                DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            DrawTextW(bufDc, heightBuf, -1, &heightTextRc,
                                DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            if (centerFits && fontCenter) {
                SelectObject(bufDc, fontCenter);
                DrawTextW(bufDc, centerBuf, -1, &centerTextRc,
                                    DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            SelectObject(bufDc, oldFontDim);
            if (fontCenter)
                DeleteObject(fontCenter);
            DeleteObject(fontDim);
        }
    }
}

void DrawCrosshairAndCoordTooltip(HDC bufDc, HBITMAP bufBmp, HWND hwnd,
                                                                    int w, int h, int cx, int cy, bool dragging) {
    if (cx < 0 || cx >= w || cy < 0 || cy >= h)
        return;

    if (!dragging) {
        // Monitor bounds in client coords (for coord tooltip and magnifier).
        POINT screenPt;
        GetCursorPos(&screenPt);
        HMONITOR hMon = MonitorFromPoint(screenPt, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        int monLeft = 0, monTop = 0, monRight = w, monBottom = h;
        if (GetMonitorInfoW(hMon, &mi)) {
            RECT const& r = mi.rcMonitor;
            POINT tl = {r.left, r.top}, br = {r.right, r.bottom};
            ScreenToClient(hwnd, &tl);
            ScreenToClient(hwnd, &br);
            monLeft = tl.x;
            monTop = tl.y;
            monRight = br.x;
            monBottom = br.y;
        }

        HPEN crossPen = CreatePen(PS_SOLID, 1, kCrosshairColor);
        if (crossPen) {
            HGDIOBJ oldPen = SelectObject(bufDc, crossPen);
            for (int y = 0; y < h; y += 2) {
                MoveToEx(bufDc, cx, y, nullptr);
                LineTo(bufDc, cx, y + 1);
            }
            for (int x = 0; x < w; x += 2) {
                MoveToEx(bufDc, x, cy, nullptr);
                LineTo(bufDc, x + 1, cy);
            }
            SelectObject(bufDc, oldPen);
            DeleteObject(crossPen);
        }

        // Round magnifier (drawn before coord tooltip so coords stay on top).
        int const srcX = cx - kMagnifierSource / 2;
        int const srcY = cy - kMagnifierSource / 2;
        bool const sourceInBounds =
                srcX >= 0 && srcY >= 0 &&
                srcX + kMagnifierSource <= w && srcY + kMagnifierSource <= h;
        if (sourceInBounds) {
            int const pad = kMagnifierPadding;
            int magLeft;
            int magTop;
            if (cx + pad + kMagnifierSize <= monRight &&
                    cy + pad + kMagnifierSize <= monBottom) {
                magLeft = cx + pad;
                magTop = cy + pad;
            } else if (cx - pad - kMagnifierSize >= monLeft &&
                                  cy + pad + kMagnifierSize <= monBottom) {
                magLeft = cx - pad - kMagnifierSize;
                magTop = cy + pad;
            } else if (cx + pad + kMagnifierSize <= monRight &&
                                  cy - pad - kMagnifierSize >= monTop) {
                magLeft = cx + pad;
                magTop = cy - pad - kMagnifierSize;
            } else if (cx - pad - kMagnifierSize >= monLeft &&
                                  cy - pad - kMagnifierSize >= monTop) {
                magLeft = cx - pad - kMagnifierSize;
                magTop = cy - pad - kMagnifierSize;
            } else {
                magLeft = cx + pad;
                magTop = cy + pad;
                if (magLeft < monLeft)
                    magLeft = monLeft;
                if (magTop < monTop)
                    magTop = monTop;
                if (magLeft + kMagnifierSize > monRight)
                    magLeft = monRight - kMagnifierSize;
                if (magTop + kMagnifierSize > monBottom)
                    magTop = monBottom - kMagnifierSize;
            }
            HRGN rgn =
                    CreateEllipticRgn(magLeft, magTop, magLeft + kMagnifierSize,
                                                        magTop + kMagnifierSize);
            if (rgn) {
                SelectClipRgn(bufDc, rgn);
                SetStretchBltMode(bufDc, COLORONCOLOR);
                StretchBlt(bufDc, magLeft, magTop, kMagnifierSize, kMagnifierSize,
                                    bufDc, srcX, srcY, kMagnifierSource, kMagnifierSource,
                                    SRCCOPY);
                SelectClipRgn(bufDc, nullptr);
                DeleteObject(rgn);
            }
            HPEN magBorderPen = CreatePen(PS_SOLID, 1, kCoordTooltipBorderText);
            if (magBorderPen) {
                HGDIOBJ oldPen = SelectObject(bufDc, magBorderPen);
                SelectObject(bufDc, GetStockObject(NULL_BRUSH));
                Ellipse(bufDc, magLeft, magTop, magLeft + kMagnifierSize,
                              magTop + kMagnifierSize);
                SelectObject(bufDc, oldPen);
                DeleteObject(magBorderPen);
            }
        }

        constexpr int kCoordPadding = 4;
        constexpr int kCoordMargin = 4;
        wchar_t coordBuf[32];
        swprintf_s(coordBuf, L"%d x %d", cx, cy);
        HFONT font =
                CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        DEFAULT_QUALITY, FF_DONTCARE, L"Segoe UI");
        if (font) {
            HGDIOBJ oldFont = SelectObject(bufDc, font);
            SIZE textSize = {};
            if (GetTextExtentPoint32W(bufDc, coordBuf,
                                                                static_cast<int>(wcslen(coordBuf)),
                                                                &textSize)) {
                int boxW = textSize.cx + 2 * kCoordMargin;
                int boxH = textSize.cy + 2 * kCoordMargin;
                int left = cx + kCoordPadding;
                int top = cy + kCoordPadding;
                if (left + boxW > monRight)
                    left = cx - kCoordPadding - boxW;
                if (top + boxH > monBottom)
                    top = cy - kCoordPadding - boxH;
                if (left < monLeft)
                    left = monLeft;
                if (top < monTop)
                    top = monTop;
                greenflame::core::RectPx boxRect =
                        greenflame::core::RectPx::FromLtrb(left, top, left + boxW, top + boxH);
                int const rowBytes = greenflame::RowBytes32(w);
                size_t const pixSize =
                        static_cast<size_t>(rowBytes) * static_cast<size_t>(h);
                std::vector<uint8_t> tooltipPixels(pixSize);
                BITMAPINFOHEADER bmi;
                greenflame::FillBmi32TopDown(bmi, w, h);
                if (GetDIBits(bufDc, bufBmp, 0, static_cast<UINT>(h),
                                          tooltipPixels.data(),
                                          reinterpret_cast<BITMAPINFO*>(&bmi),
                                          DIB_RGB_COLORS) != 0) {
                    greenflame::core::BlendRectOntoPixels(
                            tooltipPixels, w, h, rowBytes, boxRect, kCoordTooltipBgR,
                            kCoordTooltipBgG, kCoordTooltipBgB, kCoordTooltipAlpha);
                    SetDIBits(bufDc, bufBmp, 0, static_cast<UINT>(h),
                                        tooltipPixels.data(),
                                        reinterpret_cast<BITMAPINFO*>(&bmi), DIB_RGB_COLORS);
                }
                HPEN borderPen = CreatePen(PS_SOLID, 1, kCoordTooltipBorderText);
                if (borderPen) {
                    HGDIOBJ oldPen = SelectObject(bufDc, borderPen);
                    SelectObject(bufDc, GetStockObject(NULL_BRUSH));
                    Rectangle(bufDc, left, top, left + boxW, top + boxH);
                    SelectObject(bufDc, oldPen);
                    DeleteObject(borderPen);
                }
                SetBkMode(bufDc, TRANSPARENT);
                SetTextColor(bufDc, kCoordTooltipBorderText);
                RECT textRc = {left + kCoordMargin, top + kCoordMargin,
                                              left + boxW - kCoordMargin, top + boxH - kCoordMargin};
                DrawTextW(bufDc, coordBuf, -1, &textRc,
                                    DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }
            SelectObject(bufDc, oldFont);
            DeleteObject(font);
        }
    }
}

}  // namespace

namespace greenflame {

void PaintOverlay(HDC hdc, HWND hwnd, const RECT& rc,
                                    const PaintOverlayInput& in) {
    int const w = rc.right - rc.left;
    int const h = rc.bottom - rc.top;

    if (!in.capture || !in.capture->IsValid() || w <= 0 || h <= 0) {
        HBRUSH brush = CreateSolidBrush(RGB(0x40, 0x40, 0x40));
        if (brush) {
            FillRect(hdc, &rc, brush);
            DeleteObject(brush);
        }
        return;
    }

    HDC bufDc = CreateCompatibleDC(hdc);
    HBITMAP bufBmp = CreateCompatibleBitmap(hdc, w, h);
    if (!bufDc || !bufBmp) {
        if (bufBmp)
            DeleteObject(bufBmp);
        if (bufDc)
            DeleteDC(bufDc);
        return;
    }

    HGDIOBJ oldBuf = SelectObject(bufDc, bufBmp);

    DrawCaptureToBuffer(bufDc, hdc, w, h, in.capture);

    greenflame::core::RectPx sel =
            in.modifier_preview
                    ? in.live_rect
                    : (in.handle_dragging ? in.live_rect
                                                                : (in.dragging ? in.live_rect
                                                                                                : in.final_selection));
    DrawSelectionDimAndBorder(bufDc, bufBmp, w, h, sel);

    if ((in.dragging || in.handle_dragging || in.modifier_preview) &&
            !sel.IsEmpty())
        DrawDimensionLabels(bufDc, bufBmp, w, h, sel);

    bool const show_crosshair = in.final_selection.IsEmpty() && !in.dragging &&
                                                            !in.handle_dragging && !in.modifier_preview;
    int const cx = in.cursor_client_px.x;
    int const cy = in.cursor_client_px.y;
    if (show_crosshair)
        DrawCrosshairAndCoordTooltip(bufDc, bufBmp, hwnd, w, h, cx, cy, false);
    // Resize handles only when committed or resizing; never in Object_selection (modifier_preview).
    else if (in.handle_dragging && !in.live_rect.IsEmpty())
        DrawContourHandles(bufDc, in.live_rect);
    else if (!in.final_selection.IsEmpty() && !in.modifier_preview)
        DrawContourHandles(bufDc, in.final_selection);

    BitBlt(hdc, 0, 0, w, h, bufDc, 0, 0, SRCCOPY);
    SelectObject(bufDc, oldBuf);
    DeleteObject(bufBmp);
    DeleteDC(bufDc);
}

}  // namespace greenflame
