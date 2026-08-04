#pragma once
#include <gui/Canvas.h>
#include <gui/Image.h>
#include <gui/Shape.h>
#include <gui/DrawableString.h>
#include <gui/Font.h>
#include "ImageData.h"

// ============================================================
// DenoisingView — three-panel canvas
// Draws: [Original] | [Noisy] | [Denoised] side by side.
//
// WHY gui::Image instead of gui::Texture:
//   gui::Texture requires an active OpenGL GL context, which
//   GTK/natID only provides when the Canvas is the direct
//   central view (not inside any layout).  Inside a layout,
//   GL context initialisation aborts on Windows/GTK.
//   gui::Image uses the platform 2D drawing API — no GL
//   context required — so Canvas works inside HorizontalLayout
//   exactly as in the natID lab4-8-puzzle example.
//
// WHY reDraw() instead of startAnimation():
//   reDraw() is the correct call for on-demand repaints.
//   startAnimation() was a workaround for a historical
//   LNK2019 issue that no longer applies.
//
// SRP: rendering only — no solver, no business logic.
// ============================================================
class DenoisingView : public gui::Canvas
{
private:
    gui::Image _imgOriginal;
    gui::Image _imgDenoised;

    bool _hasOriginal  = false;
    bool _hasDenoised  = false;

    static constexpr float kLabelH = 24.0f;
    static constexpr float kPad    = 8.0f;

protected:
    void onDraw(const gui::Rect& rect) override
    {
        // Clear background
        gui::Shape bg;
        bg.createRect(rect);
        bg.drawFill(td::ColorID::White);

        if (!_hasOriginal) return;

        const float totalW = rect.right  - rect.left;
        const float totalH = rect.bottom - rect.top;
        const float panelW = totalW / 2.0f;

        const char*  labels[2] = { "Original", "Denoised" };
        gui::Image*  imgs[2]   = { &_imgOriginal, &_imgDenoised };
        bool         has[2]    = { _hasOriginal,  _hasDenoised  };

        for (int col = 0; col < 2; ++col)
        {
            float x = rect.left + col * panelW;

            // Thin divider between the two panels
            if (col == 1)
            {
                gui::Shape div;
                div.createRect(gui::Rect(x, rect.top, x + 1.0f, rect.bottom));
                div.drawFill(td::ColorID::LightGray);
            }

            // Column label
            td::String lbl(labels[col]);
            gui::Point lblPt;
            lblPt.x = x + kPad;
            lblPt.y = rect.top + 4.0f;
            gui::DrawableString::draw(lbl, lblPt,
                                      gui::Font::ID::SystemBold,
                                      td::ColorID::SysText);

            // Image content
            if (has[col])
            {
                gui::Rect imgRect(
                    x + kPad,
                    rect.top + kLabelH,
                    x + panelW - kPad,
                    rect.top + totalH - kPad
                );
                imgs[col]->draw(imgRect, gui::Image::AspectRatio::Keep);
            }
        }
    }

    void onResize(const gui::Size& /*sz*/) override
    {
        if (_hasOriginal) reDraw();
    }

public:
    DenoisingView()
    : gui::Canvas()
    {
        enableResizeEvent(true);
        setSizeLimits(600, gui::Control::Limit::UseAsMin,
                      400, gui::Control::Limit::UseAsMin);
    }

    // Load original directly from disk path (no temp file needed).
    void setOriginalFromFile(const std::string& path)
    {
        _hasOriginal = _imgOriginal.load(path.c_str());
        reDraw();
    }

    // Set denoised image from computed pixel data.
    // Writes a uniquely-named temp PNG each call so gui::Image
    // cannot serve a stale cached version of an earlier result.
    void setDenoised(const ImageData& img)
    {
        _hasDenoised = img.toImage(_imgDenoised, "tvden_out");
        reDraw();
    }

    // Reset right panel when a new image is selected.
    void clearDenoised()
    {
        _hasDenoised = false;
        reDraw();
    }
};
