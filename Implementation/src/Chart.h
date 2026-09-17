#pragma once
#include <gui/Shape.h>
#include <gui/DrawableString.h>
#include <gui/Transformation.h>
#include "PlotStyle.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

// ============================================================
// Chart — minimal 2-D chart painter for gui::Canvas::onDraw.
//
// The natID plot library ships without a runtime library for
// Windows in this SDK, so charts are drawn with gui::Shape and
// gui::DrawableString directly.  Supports linear and log10 axes,
// grid, ticks, legend, polylines with markers, reference lines.
//
// Usage inside onDraw:
//   Chart c(rect, Scale::Log, Scale::Log);
//   c.setLabels("title", "x", "y");
//   c.fit(series);
//   c.drawFrame();
//   for (auto& s : series) c.drawSeries(s);
//   c.drawLegend(series);
// ============================================================
namespace plot
{

enum class Scale { Linear, Log };

struct PlotSeries
{
    std::vector<double> x, y;
    SeriesStyle style = kGDStyle;
    td::String  label;
    bool markers = true;
    bool line    = true;
};

class Chart
{
    gui::Rect _frame;
    gui::Rect _plot;
    Scale  _xs, _ys;
    double _x0 = 0, _x1 = 1, _y0 = 0, _y1 = 1;     // transformed units (log10 for log axes)
    bool   _xInteger = false;
    td::String _title, _xLabel, _yLabel;

    static constexpr double kLeft = 62, kRight = 14, kTop = 46, kBottom = 42;

    static bool usable(double v, Scale s) { return std::isfinite(v) && (s == Scale::Linear || v > 0); }
    static double tf(double v, Scale s) { return s == Scale::Log ? std::log10(v) : v; }

    static double niceStep(double range, int maxTicks)
    {
        const double raw = range / (std::max)(1, maxTicks);
        const double mag = std::pow(10.0, std::floor(std::log10(raw)));
        const double n = raw / mag;
        return (n < 1.5 ? 1.0 : n < 3.0 ? 2.0 : n < 7.0 ? 5.0 : 10.0) * mag;
    }

    static td::String tickLabel(double t, Scale s)
    {
        if (s == Scale::Log)
        {
            const int e = (int) std::lround(t);
            if (e >= -2 && e <= 3) return fmt("%g", std::pow(10.0, e));
            return fmt("1e%d", e);
        }
        if (std::abs(t) < 1e-12) return td::String("0");
        return fmt("%g", t);
    }

    std::vector<double> ticks(double lo, double hi, Scale s, bool integer, int maxTicks) const
    {
        std::vector<double> out;
        if (!(hi > lo)) return out;
        if (s == Scale::Log)
        {
            const int a = (int) std::ceil(lo - 1e-9), b = (int) std::floor(hi + 1e-9);
            const int step = (std::max)(1, (int) std::ceil((b - a + 1) / (double) maxTicks));
            for (int e = a; e <= b; e += step) out.push_back(e);
            return out;
        }
        double step = niceStep(hi - lo, maxTicks);
        if (integer) step = (std::max)(1.0, std::round(step));
        const double first = std::ceil(lo / step - 1e-9);
        for (int i = 0; i < 50; ++i)
        {
            const double t = (first + i) * step;
            if (t > hi + 1e-9 * step) break;
            out.push_back(t);
        }
        return out;
    }

    static void expandRange(double& lo, double& hi, Scale s)
    {
        if (!std::isfinite(lo) || !std::isfinite(hi)) { lo = 0; hi = 1; return; }
        if (s == Scale::Log)
        {
            lo = std::floor(lo); hi = std::ceil(hi);
            if (hi <= lo) hi = lo + 1;
            return;
        }
        if (hi <= lo) { lo -= 0.5; hi += 0.5; return; }
        const double pad = 0.05 * (hi - lo);
        lo -= pad; hi += pad;
    }

public:
    Chart(const gui::Rect& frame, Scale xs, Scale ys)
    : _frame(frame), _xs(xs), _ys(ys)
    {
        _plot = gui::Rect(frame.left + kLeft, frame.top + kTop, frame.right - kRight, frame.bottom - kBottom);
        if (_plot.right < _plot.left + 10) _plot.right = _plot.left + 10;
        if (_plot.bottom < _plot.top + 10) _plot.bottom = _plot.top + 10;
    }

    void setLabels(const td::String& title, const td::String& xLabel, const td::String& yLabel)
    {
        _title = title; _xLabel = xLabel; _yLabel = yLabel;
    }

    void setIntegerX(bool b) { _xInteger = b; }

    const gui::Rect& plotRect() const { return _plot; }

    // data-unit ranges
    void setXRange(double lo, double hi) { _x0 = tf(lo, _xs); _x1 = tf(hi, _xs); }
    void setYRange(double lo, double hi) { _y0 = tf(lo, _ys); _y1 = tf(hi, _ys); }

    void fit(const std::vector<PlotSeries>& series, const std::vector<double>& extraY = {})
    {
        double xl = kInfD(), xh = -kInfD(), yl = kInfD(), yh = -kInfD();
        for (auto& s : series)
            for (size_t i = 0; i < s.x.size() && i < s.y.size(); ++i)
            {
                if (!usable(s.x[i], _xs) || !usable(s.y[i], _ys)) continue;
                xl = (std::min)(xl, tf(s.x[i], _xs)); xh = (std::max)(xh, tf(s.x[i], _xs));
                yl = (std::min)(yl, tf(s.y[i], _ys)); yh = (std::max)(yh, tf(s.y[i], _ys));
            }
        for (double v : extraY)
            if (usable(v, _ys)) { yl = (std::min)(yl, tf(v, _ys)); yh = (std::max)(yh, tf(v, _ys)); }
        expandRange(xl, xh, _xs);
        expandRange(yl, yh, _ys);
        _x0 = xl; _x1 = xh; _y0 = yl; _y1 = yh;
    }

    static double kInfD() { return std::numeric_limits<double>::infinity(); }

    // transformed-units ranges (used by the landscape to keep equal aspect)
    void setRawRanges(double x0, double x1, double y0, double y1) { _x0 = x0; _x1 = x1; _y0 = y0; _y1 = y1; }

    bool toPixel(double x, double y, gui::Point& p) const
    {
        if (!usable(x, _xs) || !usable(y, _ys)) return false;
        const double tx = tf(x, _xs), ty = tf(y, _ys);
        p.x = _plot.left + (tx - _x0) / (_x1 - _x0) * _plot.width();
        p.y = _plot.bottom - (ty - _y0) / (_y1 - _y0) * _plot.height();
        return std::isfinite(p.x) && std::isfinite(p.y);
    }

    void drawFrame(bool grid = true) const
    {
        const auto fontSmall = gui::Font::ID::SystemSmaller;

        if (_title.length() > 0)
            gui::DrawableString::draw(_title, gui::Rect(_frame.left + 4, _frame.top + 4, _frame.right - 4, _frame.top + 24),
                                      gui::Font::ID::SystemBold, textColor(), td::TextAlignment::Center);

        // y label above the axis (horizontal, reads on every platform)
        if (_yLabel.length() > 0)
            gui::DrawableString::draw(_yLabel, gui::Rect(_frame.left + 4, _plot.top - 22, _plot.right, _plot.top - 8),
                                      fontSmall, textColor(), td::TextAlignment::Left);

        if (_xLabel.length() > 0)
            gui::DrawableString::draw(_xLabel, gui::Rect(_plot.left, _plot.bottom + 20, _plot.right, _frame.bottom - 2),
                                      fontSmall, textColor(), td::TextAlignment::Center);

        for (double t : ticks(_x0, _x1, _xs, _xInteger && _xs == Scale::Linear, 8))
        {
            const double px = _plot.left + (t - _x0) / (_x1 - _x0) * _plot.width();
            if (grid) gui::Shape::drawLine({ px, _plot.top }, { px, _plot.bottom }, td::ColorID::LightGray, 1, td::LinePattern::Dot);
            gui::Shape::drawLine({ px, _plot.bottom }, { px, _plot.bottom + 4 }, textColor(), 1);
            gui::DrawableString::draw(tickLabel(t, _xs), gui::Rect(px - 40, _plot.bottom + 5, px + 40, _plot.bottom + 20),
                                      fontSmall, textColor(), td::TextAlignment::Center);
        }
        for (double t : ticks(_y0, _y1, _ys, false, 6))
        {
            const double py = _plot.bottom - (t - _y0) / (_y1 - _y0) * _plot.height();
            if (grid) gui::Shape::drawLine({ _plot.left, py }, { _plot.right, py }, td::ColorID::LightGray, 1, td::LinePattern::Dot);
            gui::Shape::drawLine({ _plot.left - 4, py }, { _plot.left, py }, textColor(), 1);
            gui::DrawableString::draw(tickLabel(t, _ys), gui::Rect(_frame.left, py - 8, _plot.left - 6, py + 8),
                                      fontSmall, textColor(), td::TextAlignment::Right, td::VAlignment::Center);
        }

        gui::Shape::drawRect(_plot, textColor(), 1);
    }

    void beginClip() const
    {
        gui::Transformation::saveContext();
        gui::Transformation::setClip(_plot);
    }

    void endClip() const
    {
        gui::Transformation::restoreContext();
    }

    void drawMarker(const gui::Point& p, const SeriesStyle& st, double r = 3.5) const
    {
        if (st.squareMarker)
            gui::Shape::drawRect(gui::Rect(p.x - r, p.y - r, p.x + r, p.y + r), st.color);
        else
        {
            gui::Shape c;
            c.createCircle(gui::Circle(p, r), 1);
            c.drawFillAndWire(st.color, st.color);
        }
    }

    void drawSeries(const PlotSeries& s) const
    {
        beginClip();
        std::vector<gui::Point> run;
        auto flush = [&]()
        {
            if (s.line && run.size() >= 2)
            {
                gui::Shape poly;
                poly.createPolyLine(run.data(), run.size(), s.style.width, s.style.pattern);
                poly.drawWire(s.style.color);
            }
            run.clear();
        };

        std::vector<gui::Point> marks;
        for (size_t i = 0; i < s.x.size() && i < s.y.size(); ++i)
        {
            gui::Point p;
            if (toPixel(s.x[i], s.y[i], p)) { run.push_back(p); marks.push_back(p); }
            else flush();
        }
        flush();
        if (s.markers)
            for (auto& p : marks) drawMarker(p, s.style);
        endClip();
    }

    void drawHLine(double y, td::ColorID color, td::LinePattern pattern, const td::String& label = td::String()) const
    {
        gui::Point a, b;
        const double xmid = _xs == Scale::Log ? std::pow(10.0, _x0) : _x0;
        if (!toPixel(xmid, y, a)) return;
        if (a.y < _plot.top || a.y > _plot.bottom) return;
        gui::Shape::drawLine({ _plot.left, a.y }, { _plot.right, a.y }, color, 1.2f, pattern);
        if (label.length() > 0)
            gui::DrawableString::draw(label, gui::Rect(_plot.left + 4, a.y - 16, _plot.right - 4, a.y - 1),
                                      gui::Font::ID::SystemSmaller, color, td::TextAlignment::Right);
        (void) b;
    }

    void drawVLine(double x, td::ColorID color, td::LinePattern pattern, float width = 1.2f) const
    {
        gui::Point a;
        const double ymid = _ys == Scale::Log ? std::pow(10.0, _y0) : _y0;
        if (!toPixel(x, ymid, a)) return;
        if (a.x < _plot.left || a.x > _plot.right) return;
        gui::Shape::drawLine({ a.x, _plot.top }, { a.x, _plot.bottom }, color, width, pattern);
    }

    // Line segment in data coordinates (clipped).
    void drawSegment(double xa, double ya, double xb, double yb, td::ColorID color, td::LinePattern pattern, float width) const
    {
        gui::Point a, b;
        if (!toPixel(xa, ya, a) || !toPixel(xb, yb, b)) return;
        beginClip();
        gui::Shape::drawLine(a, b, color, width, pattern);
        endClip();
    }

    void drawLegend(const std::vector<PlotSeries>& series, bool right = true, bool bottom = false) const
    {
        const double rowH = 17, sample = 26, padding = 6;
        double w = 0;
        int n = 0;
        for (auto& s : series)
        {
            if (s.label.length() == 0) continue;
            gui::Size sz;
            gui::DrawableString::measure(s.label, sz, gui::Font::ID::SystemSmaller);
            w = (std::max)(w, sz.width);
            // natID text measurement can under-report widths (and does during PDF/SVG export)
            w = (std::max)(w, 6.5 * (double) s.label.length());
            ++n;
        }
        if (n == 0) return;
        const double boxW = w + sample + 3 * padding, boxH = n * rowH + 2 * padding - 2;
        const double left = right ? _plot.right - boxW - 8 : _plot.left + 8;
        const double top = bottom ? _plot.bottom - boxH - 8 : _plot.top + 8;
        const gui::Rect box(left, top, left + boxW, top + boxH);
        gui::Shape::drawRect(box, 0.85f, panelColor());
        gui::Shape::drawRect(box, td::ColorID::Gray, 1);

        double y = top + padding;
        for (auto& s : series)
        {
            if (s.label.length() == 0) continue;
            const double cy = y + rowH / 2 - 1;
            if (s.line)
                gui::Shape::drawLine({ left + padding, cy }, { left + padding + sample, cy }, s.style.color, s.style.width, s.style.pattern);
            if (s.markers)
                drawMarker({ left + padding + sample / 2, cy }, s.style, 3.0);
            gui::DrawableString::draw(s.label, gui::Rect(left + 2 * padding + sample, y, box.right - 2, y + rowH),
                                      gui::Font::ID::SystemSmaller, textColor(), td::TextAlignment::Left, td::VAlignment::Center,
                                      td::TextEllipsize::None);
            y += rowH;
        }
    }

    void drawMessage(const td::String& msg) const
    {
        gui::DrawableString::draw(msg, _plot, gui::Font::ID::SystemNormal, textColor(),
                                  td::TextAlignment::Center, td::VAlignment::Center);
    }
};

} // namespace plot
