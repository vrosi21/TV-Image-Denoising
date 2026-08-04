#pragma once
#include <gui/StatusBar.h>
#include <gui/Label.h>

// ============================================================
// AppStatusBar — read-only status strip at the bottom.
//
// NatID StatusBar reliably renders only Labels (and
// ProgressIndicators) — NOT Buttons or Sliders.  This bar
// therefore uses Labels only: one for the current image name,
// one for the denoising parameters last used.
//
// SRP: displays status information; no business logic.
// ============================================================
class AppStatusBar : public gui::StatusBar
{
private:
    gui::Label _lblImage;
    gui::Label _lblParams;

public:
    AppStatusBar()
    : gui::StatusBar(4)
    , _lblImage("No image loaded")
    , _lblParams("")
    {
        _lblImage.setResizable();

        _layout.setSpaceBetweenCells(8);
        _layout << _lblImage;
        _layout.appendSpace(16);
        _layout << _lblParams;

        setLayout(&_layout);
    }

    // Called by MainWindow when a new image is loaded.
    void setImageName(const char* name)
    {
        _lblImage.setTitle(td::String(name));
    }

    // Called by MainWindow after a successful denoise run.
    void setParams(const td::String& txt)
    {
        _lblParams.setTitle(txt);
    }
};
