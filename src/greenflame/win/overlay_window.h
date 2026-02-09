#pragma once

#include <windows.h>

namespace greenflame {

// Register the overlay window class. Call once at startup before CreateOverlayIfNone.
bool RegisterOverlayClass(HINSTANCE hInstance);

// Create and show the overlay if none exists; no-op if an overlay is already
// active. Captures the virtual desktop and runs the selection flow.
void CreateOverlayIfNone(HINSTANCE hInstance);

}  // namespace greenflame
