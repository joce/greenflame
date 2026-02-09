#include "dpi.h"
#include <windows.h>

namespace greenflame
{
        void EnablePerMonitorDpiAwarenessV2()
        {
                // Best-effort: if manifest already sets it, this is redundant but safe.
                // We avoid hard-failing if the API is unavailable.
                HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
                if (!user32) return;

                using SetDpiAwarenessContextFn = DPI_AWARENESS_CONTEXT (WINAPI*)(DPI_AWARENESS_CONTEXT);
                auto fn = reinterpret_cast<SetDpiAwarenessContextFn>(
                        ::GetProcAddress(user32, "SetProcessDpiAwarenessContext")
                );

                if (fn)
                {
                        fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
                }
        }
}
