#pragma once
#include "PlotCanvas.h"
#include <algorithm>
#include <limits>
#include <vector>

// ============================================================
// ConvergenceView: per-iteration behaviour of the solvers.
//
//   top-left    : energy gap F(u_k) − F_best (log), linear vs.
//                 fast convergence of GD and Newton
//   top-right   : PSNR of u_k against the clean image
//   bottom-left : relative change ‖u_k − u_(k−1)‖ / ‖u_(k−1)‖ (log)
//   bottom-right: PSNR against wall time, accuracy per unit of work
//
// F uses the exact TV (epsilon = 0) so both solvers are measured
// with the same yardstick.
// ============================================================
class ConvergenceView : public PlotCanvas
{
    using PlotSeries = plot::PlotSeries;
    using Chart = plot::Chart;
    using Scale = plot::Scale;

    template <typename FX, typename FY>
    std::vector<PlotSeries> collect(FX xOf, FY yOf) const
    {
        std::vector<PlotSeries> out;
        for (auto& run : _result->runs)
        {
            PlotSeries s;
            s.style = plot::styleFor(run.kind);
            s.label = td::String(solverName(run.kind));
            for (auto& rec : run.iterations)
            {
                s.x.push_back(xOf(rec));
                s.y.push_back(yOf(rec));
            }
            out.push_back(s);
        }
        return out;
    }

    void drawChart(const gui::Rect& rect, std::vector<PlotSeries> series, Scale xs, Scale ys, bool integerX,
                   const td::String& title, const td::String& xLabel, const td::String& yLabel,
                   const std::vector<double>& extraY = {}, bool legendBottom = false)
    {
        Chart c(rect, xs, ys);
        c.setIntegerX(integerX);
        c.setLabels(title, xLabel, yLabel);
        c.fit(series, extraY);
        c.drawFrame();
        for (auto& s : series) c.drawSeries(s);
        c.drawLegend(series, true, legendBottom);
    }

protected:
    void paint(const gui::Rect& r) override
    {
        if (!_result || _result->runs.empty())
        {
            paintHint(r, "Press Run to compare the methods iteration by iteration.");
            return;
        }

        const double gap = 10;
        const double midX = (r.left + r.right) / 2, midY = (r.top + r.bottom) / 2;
        const gui::Rect tl(r.left + 6, r.top + 6, midX - gap / 2, midY - gap / 2);
        const gui::Rect tr(midX + gap / 2, r.top + 6, r.right - 6, midY - gap / 2);
        const gui::Rect bl(r.left + 6, midY + gap / 2, midX - gap / 2, r.bottom - 6);
        const gui::Rect br(midX + gap / 2, midY + gap / 2, r.right - 6, r.bottom - 6);

        // --- energy gap ------------------------------------------------
        double fBest = std::numeric_limits<double>::infinity();
        for (auto& run : _result->runs)
            for (auto& rec : run.iterations) fBest = (std::min)(fBest, rec.energy);
        auto gapSeries = collect([](const IterationRecord& rec) { return (double) rec.k; },
                                 [fBest](const IterationRecord& rec) { return rec.energy - fBest; });
        drawChart(tl, gapSeries, Scale::Linear, Scale::Log, true,
                  plot::fmt("Energy gap  F(u_k) %s F_best   (%s = %.3g)", plot::glyph::minus, plot::glyph::lambda, _result->settings.lambda),
                  td::String("iteration k"), td::String("F(u_k) - F_best"));

        // --- PSNR per iteration ------------------------------------------
        auto psnrSeries = collect([](const IterationRecord& rec) { return (double) rec.k; },
                                  [](const IterationRecord& rec) { return rec.psnr; });
        {
            Chart c(tr, Scale::Linear, Scale::Linear);
            c.setIntegerX(true);
            c.setLabels(td::String("Image quality"), td::String("iteration k"), td::String("PSNR [dB]"));
            c.fit(psnrSeries, { _result->noisyPsnr });
            c.drawFrame();
            c.drawHLine(_result->noisyPsnr, plot::kGuideColor, td::LinePattern::Dash, td::String("noisy input"));
            for (auto& s : psnrSeries) c.drawSeries(s);
            c.drawLegend(psnrSeries, true, true);
        }

        // --- relative change ---------------------------------------------
        auto relSeries = collect([](const IterationRecord& rec) { return (double) rec.k; },
                                 [](const IterationRecord& rec) { return rec.k == 0 ? std::nan("") : rec.relChange; });
        drawChart(bl, relSeries, Scale::Linear, Scale::Log, true,
                  td::String("Step size between iterates"), td::String("iteration k"),
                  plot::fmt("%su_k - u_k-1%s / %su_k-1%s", plot::glyph::norm, plot::glyph::norm, plot::glyph::norm, plot::glyph::norm));

        // --- PSNR vs time -------------------------------------------------
        auto timeSeries = collect([](const IterationRecord& rec) { return rec.timeMs; },
                                  [](const IterationRecord& rec) { return rec.psnr; });
        drawChart(br, timeSeries, Scale::Linear, Scale::Linear, false,
                  td::String("Quality vs computation time"), td::String("wall time [ms]"), td::String("PSNR [dB]"),
                  { _result->noisyPsnr }, true);
    }
};
