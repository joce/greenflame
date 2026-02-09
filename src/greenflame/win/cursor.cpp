#include "cursor.h"

#include <windows.h>

namespace greenflame
{
        greenflame::core::PointPx GetCursorPosPx()
        {
                POINT pt{};
                if (GetCursorPos(&pt))
                        return greenflame::core::PointPx{pt.x, pt.y};
                return greenflame::core::PointPx{0, 0};
        }
}
