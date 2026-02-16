#pragma once

// Phase 3.1: GDI full-screen capture of the virtual desktop.
// Capture sequence: GetDC(GetDesktopWindow), CreateCompatibleDC,
// CreateDIBSection, BitBlt(SRCCOPY | CAPTUREBLT). Caller must call
// Result::Free() when done (or DeleteObject(result.bitmap)).

#include "win_min_fwd.h"

namespace greenflame {

struct GdiCaptureResult {
    HBITMAP bitmap = nullptr;
    int width = 0;
    int height = 0;

    void Free() noexcept;
    bool IsValid() const noexcept { return bitmap != nullptr && width > 0 && height > 0; }
};

// Captures the virtual desktop into a 32bpp top-down DIB.
// Returns true and fills out on success; false on failure (out is cleared).
bool CaptureVirtualDesktop(GdiCaptureResult& out);

// Writes the capture to a BMP file (for Phase 3.1 validation).
// Returns true on success. Path is UTF-16 (wchar_t).
bool SaveCaptureToBmp(GdiCaptureResult const& capture, wchar_t const* path);

// Crops source to the given rect (in source coords). For Phase 3.5 commit.
// Caller must call out.Free() when done. Returns false if rect is empty or out of bounds.
bool CropCapture(GdiCaptureResult const& source, int left, int top, int width, int height,
                                  GdiCaptureResult& out);

// --- Helpers for 32bpp top-down DIB (used by capture and overlay paint) ---
void FillBmi32TopDown(BITMAPINFOHEADER& bmi, int width, int height);
int RowBytes32(int width);

}  // namespace greenflame
