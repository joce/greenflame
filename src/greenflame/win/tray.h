#pragma once

#include <windows.h>

namespace greenflame {

// Register the tray window class. Call once at startup before CreateTrayWindow.
bool RegisterTrayClass(HINSTANCE hInstance);

// Create the tray (message-only) window and add the notification area icon.
// on_start_capture is invoked when the user chooses "Start capture" from the
// context menu; it receives the module HINSTANCE. Exit is handled internally
// (DestroyWindow, NIM_DELETE, PostQuitMessage). Returns the tray window handle
// or nullptr on failure.
HWND CreateTrayWindow(HINSTANCE hInstance,
                                            void (*on_start_capture)(HINSTANCE hInstance));

}  // namespace greenflame
