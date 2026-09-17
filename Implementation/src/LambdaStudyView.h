#pragma once
#include "PlotCanvas.h"
#include <vector>

// ============================================================
// LambdaStudyView: how the regularisation weight affects both
// methods, on identical noisy input.
//
//   left        : PSNR vs lambda (log axis) for GD and Newton,
//                 best lambda of each method marked
//   right-top   : L-curve TV(u) vs data misfit ½‖u − f‖² per method
//   right-bottom: solver time vs lambda, cost of each method
//
// The lambda currently set in the sidebar is drawn as a dotted
// guide so the user sees where the working point lies.
// ============================================================
class LambdaStudyView : public PlotCanvas
{
    using PlotSeries = plot::PlotSeries;
    using Chart = plot::Chart;
    using Scale = plot::Scale;

    double _currentLambda = 0.1;

    template <typename FX, typename FY>
    std::vector<PlotSeries> collect(FX xOf, FY yOf) const
    {
        std::vector<PlotSeries> out;
        for (auto& series : _result->study)
        {
            PlotSeries s;
            s.style = plot::styleFor(series.kind);
            s.label = td::String(solverName(series.kind));
            for (auto& p : series.points) { s.x.push_back(xOf(p)); s.y.push_back(yOf(p)); }
            out.push_back(s);
        }
        return out;
    }

    // best-lambda markers (hollow look: larger marker in the method colour)
    std::vector<PlotSeries> bestMarkers(bool lCurve) const
    {
        std::vector<PlotSeries> out;
        for (auto& series : _result->study)
        {
            const int b = series.bestIndex();
            if (b < 0) continue;
            const StudyPoint& p = series.points[(size_t) b];
            PlotSeries m;
            m.style = plot::styleFor(series.kind);
            m.line = false;
            m.x = { lCurve ? p.fidelity : p.lambda };
            m.y = { lCurve ? p.tv : p.psnr };
            m.label = plot::fmt("best %s: %s = %.3g, %.2f dB", solverShortName(series.kind),
                                plot::glyph::lambda, p.lambda, p.psnr);
            out.push_back(m);
        }
        return out;
    }

protected:
    void paint(const gui::Rect& r) override
    {
        if (!_result || _result->study.empty())
        {
            paintHint(r, "Run a lambda study to see how the regularisation weight affects both methods.");
            return;
        }

        const double gap = 10;
        const double midX = r.left + 0.52 * (r.right - r.left);
        const double midY = (r.top + r.bottom) / 2;
        const gui::Rect left(r.left + 6, r.top + 6, midX - gap / 2, r.bottom - 6);
        const gui::Rect rightTop(midX + gap / 2, r.top + 6, r.right - 6, midY - gap / 2);
        const gui::Rect rightBottom(midX + gap / 2, midY + gap / 2, r.right - 6, r.bottom - 6);

        // --- PSNR vs lambda ----------------------------------------------
        {
            auto series = collect([](const StudyPoint& p) { return p.lambda; },
                                  [](const StudyPoint& p) { return p.psnr; });
            auto best = bestMarkers(false);
            std::vector<PlotSeries> all = series;
            all.insert(all.end(), best.begin(), best.end());

            Chart c(left, Scale::Log, Scale::Linear);
            c.setLabels(plot::fmt("Image quality vs %s   (%s = %.3f, %d iterations)", plot::glyph::lambda,
                                  plot::glyph::sigma, _result->settings.sigma, _result->settings.iterations),
                        plot::fmt("%s  (regularisation weight, log scale)", plot::glyph::lambda),
                        td::String("PSNR [dB]"));
            c.fit(all, { _result->noisyPsnr });
            c.drawFrame();
            c.drawHLine(_result->noisyPsnr, plot::kGuideColor, td::LinePattern::Dash, td::String("noisy input"));
            c.drawVLine(_currentLambda, plot::textColor(), td::LinePattern::Dot);
            for (auto& s : series) c.drawSeries(s);
            c.beginClip();
            for (auto& m : best)
            {
                gui::Point p;
                if (c.toPixel(m.x[0], m.y[0], p))
                {
                    gui::Shape ring;
                    ring.createCircle(gui::Circle(p, 8.0), 2.0f);
                    ring.drawWire(m.style.color);
                }
            }
            c.endClip();

            std::vector<PlotSeries> legend = series;
            for (auto& m : best) { PlotSeries l = m; l.line = false; legend.push_back(l); }
            PlotSeries current;
            current.style = { plot::textColor(), td::LinePattern::Dot, 1.2f, false };
            current.markers = false;
            current.label = plot::fmt("current %s = %.3f", plot::glyph::lambda, _currentLambda);
            legend.push_back(current);
            c.drawLegend(legend, true, true);
        }

        // --- L-curve --------------------------------------------------------
        {
            auto series = collect([](const StudyPoint& p) { return p.fidelity; },
                                  [](const StudyPoint& p) { return p.tv; });
            Chart c(rightTop, Scale::Log, Scale::Log);
            c.setLabels(td::String("L-curve: smoothness vs data fit"),
                        plot::fmt("data misfit  %su %s f%s%s / 2", plot::glyph::norm, plot::glyph::minus, plot::glyph::norm, "\xc2\xb2"),
                        td::String("TV(u)"));
            c.fit(series);
            c.drawFrame();
            for (auto& s : series) c.drawSeries(s);
            c.drawLegend(series, true, false);
        }

        // --- cost vs lambda --------------------------------------------------
        {
            auto series = collect([](const StudyPoint& p) { return p.lambda; },
                                  [](const StudyPoint& p) { return p.timeMs; });
            Chart c(rightBottom, Scale::Log, Scale::Log);
            c.setLabels(td::String("Computation time per solve"),
                        plot::fmt("%s  (log scale)", plot::glyph::lambda), td::String("time [ms]"));
            c.fit(series);
            c.drawFrame();
            for (auto& s : series) c.drawSeries(s);
            c.drawLegend(series, true, true);
        }
    }

public:
    void setCurrentLambda(double lambda)
    {
        _currentLambda = lambda;
        reDraw();
    }
};
