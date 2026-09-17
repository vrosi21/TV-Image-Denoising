#pragma once
#include <td/ColorID.h>
#include <td/LinePattern.h>
#include <td/String.h>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include "DenoisingRunner.h"

// ============================================================
// PlotStyle: shared colours, glyphs and formatting so each
// solver looks the same in every chart, table and caption.
// ============================================================
namespace plot
{

namespace glyph
{
    inline constexpr const char* lambda = "\xce\xbb";
    inline constexpr const char* sigma  = "\xcf\x83";
    inline constexpr const char* eps    = "\xce\xb5";
    inline constexpr const char* norm   = "\xe2\x80\x96";
    inline constexpr const char* minus  = "\xe2\x88\x92";
    inline constexpr const char* dot    = "\xc2\xb7";
    inline constexpr const char* times  = "\xc3\x97";
}

struct SeriesStyle
{
    td::ColorID     color;
    td::LinePattern pattern;
    float           width;
    bool            squareMarker;
};

inline constexpr SeriesStyle kGDStyle     { td::ColorID::Crimson, td::LinePattern::Solid, 2.0f, false };
inline constexpr SeriesStyle kNewtonStyle { td::ColorID::Teal,    td::LinePattern::Solid, 2.0f, true  };
inline constexpr td::ColorID kGuideColor  = td::ColorID::Gray;
inline constexpr td::ColorID kBestColor   = td::ColorID::ForestGreen;

// ---- print mode ----------------------------------------------
// Exports (PDF/SVG) must not use theme colours: in dark mode the
// system text colour is white and would vanish on a white page.
// PlotCanvas/DenoisingView switch this on while natID renders to an
// export backend.
inline bool& printModeFlag()
{
    static thread_local bool on = false;
    return on;
}

struct PrintScope
{
    bool previous;
    explicit PrintScope(bool on) : previous(printModeFlag()) { printModeFlag() = on; }
    ~PrintScope() { printModeFlag() = previous; }
};

inline td::ColorID textColor()  { return printModeFlag() ? td::ColorID::Black : td::ColorID::SysText; }
inline td::ColorID panelColor() { return printModeFlag() ? td::ColorID::White : td::ColorID::SysCtrlBack; }

inline const SeriesStyle& styleFor(SolverKind k)
{
    return k == SolverKind::GD ? kGDStyle : kNewtonStyle;
}

inline td::String fmt(const char* format, ...)
{
    char buf[512];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buf, sizeof buf, format, args);
    va_end(args);
    return td::String(buf);
}

} // namespace plot
