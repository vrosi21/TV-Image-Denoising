#pragma once
#include <gui/Canvas.h>
#include "Chart.h"
#include "DenoisingRunner.h"

// ============================================================
// PlotCanvas — base for result views drawn with plot::Chart.
// Holds the shared immutable result; subclasses paint the whole
// canvas rectangle.
// ============================================================
class PlotCanvas : public gui::Canvas
{
protected:
    DenoiseResultPtr _result;

    virtual void paint(const gui::Rect& r) = 0;

    void onDraw(const gui::Rect&) override
    {
        gui::Size sz;
        getSize(sz);
        const bool exporting = _backend != Backend::Display;
        plot::PrintScope print(exporting);     // black text on white for PDF/SVG
        if (exporting) gui::Shape::drawRect(gui::Rect(0, 0, sz.width, sz.height), td::ColorID::White);
        paint(gui::Rect(0, 0, sz.width, sz.height));
    }

    void onResize(const gui::Size&) override
    {
        reDraw();
    }

    void paintHint(const gui::Rect& r, const char* text) const
    {
        gui::DrawableString::draw(td::String(text), r, gui::Font::ID::SystemNormal, plot::textColor(),
                                  td::TextAlignment::Center, td::VAlignment::Center);
    }

public:
    PlotCanvas()
    {
        enableResizeEvent(true);
        setSizeLimits(320, gui::Control::Limit::UseAsMin, 240, gui::Control::Limit::UseAsMin);
    }

    virtual void setResult(const DenoiseResultPtr& r)
    {
        _result = r;
        reDraw();
    }
};
