#pragma once
#include <gui/StatusBar.h>
#include <gui/Label.h>
#include <gui/ProgressIndicator.h>

// ============================================================
// AppStatusBar: read-only status strip at the bottom.
//
// NatID StatusBar reliably renders only Labels (and
// ProgressIndicators), NOT Buttons or Sliders.  This bar
// therefore uses Labels only plus a progress indicator that
// fills while a computation runs:
//   [image name]   [status / result summary ........]   [progress]
//
// SRP: displays status information; no business logic.
// ============================================================
class AppStatusBar : public gui::StatusBar
{
private:
    gui::Label             _lblImage;
    gui::Label             _lblParams;
    gui::ProgressIndicator _progress;

public:
    AppStatusBar()
    : gui::StatusBar(7)
    , _lblImage("No image loaded")
    , _lblParams("")
    {
        _lblParams.setResizable();
        _progress.setSizeLimits(160, gui::Control::Limit::Fixed);
        _progress.setValue(0.0);

        _layout.setSpaceBetweenCells(8);
        _layout.appendSpace(12);   // keeps the text off the window edge
        _layout << _lblImage;
        _layout.appendSpace(16);
        _layout << _lblParams;
        _layout.appendSpace(8);
        _layout << _progress;
        _layout.appendSpace(12);

        setLayout(&_layout);
    }

    // Called when a new image is loaded.
    void setImageName(const td::String& name)
    {
        _lblImage.setTitle(name);
    }

    // Status message or result summary.
    void setParams(const td::String& txt)
    {
        _lblParams.setTitle(txt);
    }

    // fraction in [0, 1]; a negative value resets the indicator (idle).
    void setProgress(double fraction)
    {
        _progress.setValue(fraction < 0 ? 0.0 : (fraction > 1 ? 1.0 : fraction));
    }
};
