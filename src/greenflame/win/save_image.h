#pragma once

// Save GdiCaptureResult to PNG or JPEG via Windows Imaging Component (WIC).

#include "win/gdi_capture.h"

namespace greenflame {

// Writes the capture to PNG or JPEG. Path is UTF-16 (wchar_t).
bool SaveCaptureToPng(GdiCaptureResult const &capture, wchar_t const *path);
bool SaveCaptureToJpeg(GdiCaptureResult const &capture, wchar_t const *path);

} // namespace greenflame
