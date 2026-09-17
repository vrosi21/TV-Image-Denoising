#pragma once
#include <gui/Display.h>
#include <gui/Types.h>
#include <algorithm>

// ============================================================
// WindowPlacement — initial window geometry that always fits the
// screen: the preferred size, shrunk to 90% x 85% of the display
// when the display is smaller, and centred.  Starting in a normal
// (not maximized) window avoids natID layouts that only stretch
// after the window is resized.
// ============================================================
namespace placement
{

inline gui::Geometry fitToScreen(double preferredWidth, double preferredHeight)
{
    gui::Size screen;
    gui::Display::getDefaultLogicalSize(screen);
    if (!(screen.width > 0) || !(screen.height > 0)) { screen.width = 1366; screen.height = 768; }

    const double w = (std::min)(preferredWidth, 0.90 * screen.width);
    const double h = (std::min)(preferredHeight, 0.85 * screen.height);
    const double x = (std::max)(0.0, (screen.width - w) / 2);
    const double y = (std::max)(0.0, (screen.height - h) / 2);
    return gui::Geometry(x, y, w, h);
}

} // namespace placement
