#pragma once
#include <gui/Canvas.h>
#include <gui/Image.h>
#include <gui/Shape.h>
#include <gui/DrawableString.h>
#include <gui/Font.h>
#include "ImageData.h"
#include "DenoisingRunner.h"
#include "PlotStyle.h"
#include <algorithm>
#include <cmath>
#include <vector>

// ============================================================
// DenoisingView: the Overview page.
//
//   ┌ Noisy input ┐ ┌ Gradient descent ┐ ┌ Newton (Huber) ┐   metric cards
//   └─────────────┘ └──────────────────┘ └────────────────┘
//   Original | Noisy | GD result | Newton result             image gallery
//
// The gallery picks the row/column arrangement that makes the
// images as large as possible for the current canvas size.
//
// WHY gui::Image instead of gui::Texture:
//   gui::Texture requires an active OpenGL context, which GTK/natID
//   only provides when the Canvas is the direct central view.
//   gui::Image uses the platform 2D drawing API, so the canvas also
//   works inside layouts and tab views.
//
// SRP: presentation of one experiment only, no solver logic.
// ============================================================
class DenoisingView : public gui::Canvas
{
public:
    static constexpr int kMaxPanels = 4;

private:
    struct Card
    {
        td::String  title, value, detail, badge;
        td::ColorID accent = td::ColorID::Gray;
    };

    gui::Image  _images[kMaxPanels];
    td::String  _captions[kMaxPanels];
    td::ColorID _colors[kMaxPanels] = { td::ColorID::SysText, td::ColorID::SysText, td::ColorID::SysText, td::ColorID::SysText };
    double      _aspect[kMaxPanels] = { 1, 1, 1, 1 };
    int         _count = 0;
    std::vector<Card> _cards;

    static constexpr double kCardH  = 92.0;
    static constexpr double kLabelH = 24.0;
    static constexpr double kPad    = 10.0;

    bool addPanel(const ImageData& img, const td::String& caption, td::ColorID color = td::ColorID::SysText)
    {
        if (_count >= kMaxPanels) return false;
        if (!img.toImage(_images[_count], "tvden_view")) return false;
        _captions[_count] = caption;
        _colors[_count] = color;
        _aspect[_count] = img.height > 0 ? static_cast<double>(img.width) / img.height : 1.0;
        ++_count;
        return true;
    }

    static td::String dB(double v) { return plot::fmt("%.2f dB", v); }

    void drawCard(const gui::Rect& r, const Card& c) const
    {
        gui::Shape::drawRect(r, 0.10f, c.accent);
        gui::Shape::drawRect(gui::Rect(r.left, r.top, r.left + 5, r.bottom), c.accent);
        gui::Shape::drawRect(r, td::ColorID::Silver, 1);

        const double x = r.left + 16;
        gui::DrawableString::draw(c.title, gui::Rect(x, r.top + 8, r.right - 8, r.top + 28),
                                  gui::Font::ID::SystemBold, plot::textColor());
        gui::DrawableString::draw(c.value, gui::Rect(x, r.top + 30, r.right - 8, r.top + 62),
                                  gui::Font::ID::SystemLargestBold, c.accent);
        gui::DrawableString::draw(c.detail, gui::Rect(x, r.top + 64, r.right - 8, r.bottom - 4),
                                  gui::Font::ID::SystemSmaller, plot::textColor());
        if (c.badge.length() > 0)
            gui::DrawableString::draw(c.badge, gui::Rect(r.left + 8, r.top + 8, r.right - 10, r.top + 28),
                                      gui::Font::ID::SystemSmallerBold, c.accent, td::TextAlignment::Right);
    }

    void drawGallery(const gui::Rect& area) const
    {
        const double W = area.width(), H = area.height();
        const double aspect = _aspect[0] > 0 ? _aspect[0] : 1.0;

        int bestRows = 1;
        double bestScale = -1;
        for (int rows = 1; rows <= _count; ++rows)
        {
            const int cols = (_count + rows - 1) / rows;
            const double cellW = W / cols - 2 * kPad;
            const double cellH = H / rows - kLabelH - 2 * kPad;
            const double scale = (std::min)(cellW / aspect, cellH);
            if (scale > bestScale) { bestScale = scale; bestRows = rows; }
        }
        const int rows = bestRows;
        const int cols = (_count + rows - 1) / rows;
        const double cellW = W / cols, cellH = H / rows;

        for (int i = 0; i < _count; ++i)
        {
            const int r = i / cols, c = i % cols;
            const double x = area.left + c * cellW, y = area.top + r * cellH;

            gui::DrawableString::draw(_captions[i], gui::Rect(x + kPad, y + 4, x + cellW - kPad, y + kLabelH),
                                      gui::Font::ID::SystemBold,
                                      _colors[i] == td::ColorID::SysText ? plot::textColor() : _colors[i],
                                      td::TextAlignment::Center);

            const gui::Rect imgRect(x + kPad, y + kLabelH, x + cellW - kPad, y + cellH - kPad);
            if (imgRect.width() > 4 && imgRect.height() > 4)
                _images[i].draw(imgRect, gui::Image::AspectRatio::Keep, td::HAlignment::Center, td::VAlignment::Top);
        }
    }

protected:
    void onDraw(const gui::Rect&) override
    {
        gui::Size sz;
        getSize(sz);
        const double W = sz.width, H = sz.height;

        const bool exporting = _backend != Backend::Display;
        plot::PrintScope print(exporting);
        if (exporting) gui::Shape::drawRect(gui::Rect(0, 0, W, H), td::ColorID::White);

        if (_count == 0)
        {
            gui::DrawableString::draw(td::String("Open an image to start"), gui::Rect(0, 0, W, H),
                                      gui::Font::ID::SystemLarger, plot::textColor(),
                                      td::TextAlignment::Center, td::VAlignment::Center);
            return;
        }

        double top = kPad;
        if (!_cards.empty())
        {
            // cards wrap onto more rows when the window is narrow
            const int n = (int) _cards.size();
            const double gap = 12, minCardW = 250;
            const int perRow = (std::max)(1, (std::min)(n, (int) ((W - 2 * kPad + gap) / (minCardW + gap))));
            const double cardW = (W - 2 * kPad - gap * (perRow - 1)) / perRow;
            for (int i = 0; i < n; ++i)
            {
                const double x = kPad + (i % perRow) * (cardW + gap);
                const double y = top + (i / perRow) * (kCardH + gap);
                drawCard(gui::Rect(x, y, x + cardW, y + kCardH), _cards[i]);
            }
            const int rows = (n + perRow - 1) / perRow;
            top += rows * kCardH + (rows - 1) * gap + kPad;
        }
        drawGallery(gui::Rect(0, top, W, H));
    }

    void onResize(const gui::Size&) override
    {
        reDraw();
    }

public:
    DenoisingView()
    : gui::Canvas()
    {
        enableResizeEvent(true);
        setSizeLimits(360, gui::Control::Limit::UseAsMin, 280, gui::Control::Limit::UseAsMin);
    }

    // Before any run: original and noisy preview, placeholder cards.
    void showInput(const ImageData& original, const ImageData& noisy, double noisyPsnr, const DenoiseSettings& s)
    {
        _count = 0;
        addPanel(original, td::String("Original"));
        addPanel(noisy, plot::fmt("Noisy input   %.2f dB", noisyPsnr));

        _cards.clear();
        Card input;
        input.title = td::String("Noisy input");
        input.value = dB(noisyPsnr);
        input.detail = plot::fmt("PSNR  %s  %s = %.3f  %s  seed %u", plot::glyph::dot, plot::glyph::sigma, s.sigma, plot::glyph::dot, s.seed);
        input.accent = plot::kGuideColor;
        _cards.push_back(input);

        for (SolverKind k : s.solvers())
        {
            Card c;
            c.title = td::String(solverName(k));
            c.value = td::String("-");
            c.detail = td::String("Press Run to denoise");
            c.accent = plot::styleFor(k).color;
            _cards.push_back(c);
        }
        reDraw();
    }

    void showResult(const DenoiseResult& r)
    {
        _count = 0;
        addPanel(r.original, td::String("Original"));
        addPanel(r.noisy, plot::fmt("Noisy input   %.2f dB", r.noisyPsnr));
        for (auto& run : r.runs)
            addPanel(run.result, plot::fmt("%s   %.2f dB", solverName(run.kind), run.last().psnr), plot::styleFor(run.kind).color);

        // winners among the finished runs
        const SolverRun* bestQuality = nullptr;
        const SolverRun* fastest = nullptr;
        for (auto& run : r.runs)
        {
            if (!bestQuality || run.last().psnr > bestQuality->last().psnr) bestQuality = &run;
            if (!fastest || run.totalMs < fastest->totalMs) fastest = &run;
        }

        _cards.clear();
        Card input;
        input.title = td::String("Noisy input");
        input.value = dB(r.noisyPsnr);
        input.detail = plot::fmt("PSNR  %s  %s = %.3f  %s  seed %u", plot::glyph::dot, plot::glyph::sigma, r.settings.sigma, plot::glyph::dot, r.settings.seed);
        input.accent = plot::kGuideColor;
        _cards.push_back(input);

        for (auto& run : r.runs)
        {
            Card c;
            c.title = td::String(solverName(run.kind));
            c.value = dB(run.last().psnr);
            c.detail = plot::fmt("%+.2f dB  %s  %d iterations  %s  %.0f ms",
                                 run.last().psnr - r.noisyPsnr, plot::glyph::dot, run.last().k, plot::glyph::dot, run.totalMs);
            c.accent = plot::styleFor(run.kind).color;
            if (r.runs.size() > 1)
            {
                if (&run == bestQuality && &run == fastest) c.badge = plot::fmt("BEST QUALITY %s FASTEST", plot::glyph::dot);
                else if (&run == bestQuality)               c.badge = td::String("BEST QUALITY");
                else if (&run == fastest)                   c.badge = td::String("FASTEST");
            }
            _cards.push_back(c);
        }
        if (r.cancelled)
        {
            Card stopped;
            stopped.title = td::String("Stopped");
            stopped.value = td::String("partial");
            stopped.detail = td::String("Run was cancelled before completion");
            stopped.accent = td::ColorID::DarkGoldenRod;
            _cards.push_back(stopped);
        }
        reDraw();
    }
};
