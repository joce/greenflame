#include "virtual_screen.h"
#include "greenflame_core/rect_px.h"

#include <windows.h>

namespace greenflame
{
        greenflame::core::RectPx GetVirtualDesktopBoundsPx()
        {
                const int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
                const int top = GetSystemMetrics(SM_YVIRTUALSCREEN);
                const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
                return greenflame::core::RectPxFromVirtualScreenMetrics(left, top, width, height);
        }
}
