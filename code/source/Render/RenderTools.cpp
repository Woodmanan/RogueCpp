#include "RenderTools.h"
#include "../../libraries/BearLibTerminal/Include/C/BearLibTerminal.h"
#include "Core/CoreDataTypes.h"

namespace RenderTools
{
	void DrawBresenham(int x0, int y0, int x1, int y1)
	{
        terminal_color(Color(100, 100, 0));
        terminal_bkcolor(Color(0, 0, 0));

        int _x = x0;
        int _y = y0;
        int dx = abs(x1 - x0), sx = 0 < (x1 - x0) ? 1 : -1;
        int dy = -abs(y1 - y0), sy = 0 < (y1 - y0) ? 1 : -1;
        int err = dx + dy, e2; /* error value e_xy */

        for (;;) {  /* loop */
            terminal_put(_x, _y, '*');
            if (_x == x1 && _y == y1)
            {
                terminal_put(_x, _y, '+');
                break;
            }
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; _x += sx; } /* e_xy+e_x > 0 */
            if (e2 <= dx) { err += dx; _y += sy; } /* e_xy+e_y < 0 */
        }
	}

    void DrawRect(Rect rect)
    {
        for (int x = rect.x; x < rect.x + rect.w; x++)
        {
            terminal_put(x, rect.y, L'\u2550');
            terminal_put(x, rect.y + rect.h - 1, L'\u2550');
        }

        for (int y = rect.y; y < rect.y + rect.h; y++)
        {
            terminal_put(rect.x, y, L'\u2551');
            terminal_put(rect.x + rect.w - 1, y, L'\u2551');
        }

        terminal_put(rect.x, rect.y, L'\u2554');
        terminal_put(rect.x + rect.w - 1, rect.y, L'\u2557');
        terminal_put(rect.x, rect.y + rect.h - 1, L'\u255A');
        terminal_put(rect.x + rect.w - 1, rect.y + rect.h - 1, L'\u255D');
    }
}