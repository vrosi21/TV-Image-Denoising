#pragma once
#include <gui/ToolBar.h>
#include "TBControlView.h"

// ============================================================
// MainToolBar — application toolbar.
//
// Embeds a TBControlView (gui::View with buttons + sliders)
// as a single toolbar item via addItem(label, tooltip, &view).
// This is the canonical NatID pattern for embedding custom
// controls in a toolbar (see TestGLCubeMap/TBSliderView.h).
//
// SRP: owns the toolbar shell; delegates control layout
//      and event wiring to TBControlView.
// ============================================================
class MainToolBar : public gui::ToolBar
{
private:
    TBControlView _controls;

public:
    MainToolBar()
    : gui::ToolBar("tvden_toolbar", 1)
    {
        // Embed the entire control view as one toolbar item.
        // The empty strings avoid a duplicate-label tooltip.
        addItem("", "Image selection and denoising parameters", &_controls);
    }

    TBControlView& controls() { return _controls; }
};
