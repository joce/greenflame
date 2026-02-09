// Phase 3.1: GDI full-screen capture. Virtual desktop bounds, then
// GetDC(GetDesktopWindow) -> CreateCompatibleDC -> CreateDIBSection ->
// BitBlt(SRCCOPY | CAPTUREBLT). SaveCaptureToBmp for validation.

#include "gdi_capture.h"
#include "virtual_screen.h"
#include "greenflame_core/bmp.h"
#include "greenflame_core/rect_px.h"

#include <cstdint>
#include <vector>

namespace greenflame {

namespace {

constexpr DWORD kCaptureRop = SRCCOPY | CAPTUREBLT;

}  // namespace

void FillBmi32TopDown(BITMAPINFOHEADER& bmi, int width, int height) {
    bmi = {};
    bmi.biSize = sizeof(BITMAPINFOHEADER);
    bmi.biWidth = width;
    bmi.biHeight = -height;  // top-down
    bmi.biPlanes = 1;
    bmi.biBitCount = 32;
    bmi.biCompression = BI_RGB;
}

int RowBytes32(int width) {
    return (width * 4 + 3) & ~3;
}

void GdiCaptureResult::Free() noexcept {
    if (bitmap) {
        DeleteObject(bitmap);
        bitmap = nullptr;
    }
    width = 0;
    height = 0;
}

bool CaptureVirtualDesktop(GdiCaptureResult& out) {
    out.Free();

    greenflame::core::RectPx bounds = GetVirtualDesktopBoundsPx();
    int const w = bounds.Width();
    int const h = bounds.Height();
    if (w <= 0 || h <= 0)
        return false;

    HDC const desktopDc = GetDC(GetDesktopWindow());
    if (!desktopDc)
        return false;

    HDC const memDc = CreateCompatibleDC(desktopDc);
    if (!memDc) {
        ReleaseDC(GetDesktopWindow(), desktopDc);
        return false;
    }

    BITMAPINFOHEADER bmi;
    FillBmi32TopDown(bmi, w, h);

    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(desktopDc, reinterpret_cast<BITMAPINFO*>(&bmi),
                                                                DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib || !bits) {
        DeleteDC(memDc);
        ReleaseDC(GetDesktopWindow(), desktopDc);
        return false;
    }

    HGDIOBJ const old = SelectObject(memDc, dib);
    if (!old || old == HGDI_ERROR) {
        DeleteObject(dib);
        DeleteDC(memDc);
        ReleaseDC(GetDesktopWindow(), desktopDc);
        return false;
    }

    if (!BitBlt(memDc, 0, 0, w, h, desktopDc, bounds.left, bounds.top, kCaptureRop)) {
        SelectObject(memDc, old);
        DeleteObject(dib);
        DeleteDC(memDc);
        ReleaseDC(GetDesktopWindow(), desktopDc);
        return false;
    }

    SelectObject(memDc, old);
    DeleteDC(memDc);
    ReleaseDC(GetDesktopWindow(), desktopDc);

    out.bitmap = dib;
    out.width = w;
    out.height = h;
    return true;
}

bool SaveCaptureToBmp(GdiCaptureResult const& capture, wchar_t const* path) {
    if (!capture.IsValid() || !path)
        return false;

    HDC const dc = GetDC(nullptr);
    if (!dc)
        return false;

    int const rowBytes = RowBytes32(capture.width);
    size_t const imageSize = static_cast<size_t>(rowBytes) * static_cast<size_t>(capture.height);

    BITMAPINFOHEADER info;
    FillBmi32TopDown(info, capture.width, capture.height);
    info.biHeight = capture.height;  // positive = bottom-up for BMP file

    std::vector<uint8_t> pixels(imageSize);
    if (GetDIBits(dc, capture.bitmap, 0, capture.height, pixels.data(),
                                reinterpret_cast<BITMAPINFO*>(&info), DIB_RGB_COLORS) == 0) {
        ReleaseDC(nullptr, dc);
        return false;
    }
    ReleaseDC(nullptr, dc);

    std::vector<uint8_t> bmpBytes =
            greenflame::core::BuildBmpBytes(pixels, capture.width, capture.height, rowBytes);
    if (bmpBytes.empty())
        return false;

    HANDLE const f = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE)
        return false;

    DWORD written = 0;
    bool ok = WriteFile(f, bmpBytes.data(), static_cast<DWORD>(bmpBytes.size()), &written, nullptr) &&
                        written == static_cast<DWORD>(bmpBytes.size());
    CloseHandle(f);
    return ok;
}

bool CropCapture(GdiCaptureResult const& source, int left, int top, int width, int height,
                                  GdiCaptureResult& out) {
    out.Free();
    if (width <= 0 || height <= 0 || !source.IsValid())
        return false;
    if (left < 0 || top < 0 || left + width > source.width || top + height > source.height)
        return false;

    HDC const dc = GetDC(nullptr);
    if (!dc)
        return false;

    HDC const srcDc = CreateCompatibleDC(dc);
    if (!srcDc) {
        ReleaseDC(nullptr, dc);
        return false;
    }
    HGDIOBJ const srcOld = SelectObject(srcDc, source.bitmap);
    if (!srcOld || srcOld == HGDI_ERROR) {
        DeleteDC(srcDc);
        ReleaseDC(nullptr, dc);
        return false;
    }

    BITMAPINFOHEADER bmi;
    FillBmi32TopDown(bmi, width, height);

    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(dc, reinterpret_cast<BITMAPINFO*>(&bmi),
                                                                DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib) {
        SelectObject(srcDc, srcOld);
        DeleteDC(srcDc);
        ReleaseDC(nullptr, dc);
        return false;
    }

    HDC const dstDc = CreateCompatibleDC(dc);
    if (!dstDc) {
        DeleteObject(dib);
        SelectObject(srcDc, srcOld);
        DeleteDC(srcDc);
        ReleaseDC(nullptr, dc);
        return false;
    }
    HGDIOBJ const dstOld = SelectObject(dstDc, dib);
    if (!dstOld || dstOld == HGDI_ERROR) {
        DeleteDC(dstDc);
        DeleteObject(dib);
        SelectObject(srcDc, srcOld);
        DeleteDC(srcDc);
        ReleaseDC(nullptr, dc);
        return false;
    }

    if (!BitBlt(dstDc, 0, 0, width, height, srcDc, left, top, SRCCOPY)) {
        SelectObject(dstDc, dstOld);
        DeleteDC(dstDc);
        DeleteObject(dib);
        SelectObject(srcDc, srcOld);
        DeleteDC(srcDc);
        ReleaseDC(nullptr, dc);
        return false;
    }

    SelectObject(dstDc, dstOld);
    DeleteDC(dstDc);
    SelectObject(srcDc, srcOld);
    DeleteDC(srcDc);
    ReleaseDC(nullptr, dc);

    out.bitmap = dib;
    out.width = width;
    out.height = height;
    return true;
}

}  // namespace greenflame
