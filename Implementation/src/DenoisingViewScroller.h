#pragma once
#include <gui/ViewScroller.h>
#include "DenoisingView.h"

// ============================================================
// DenoisingViewScroller — thin wrapper that lets DenoisingView
// (a Canvas) live inside a SplitterLayout.
//
// Working NatID examples (TestSplitAndPropEdit, etc.) never put
// a Canvas directly into SplitterLayout — they always wrap it in
// a ViewScroller first.  This class is that wrapper.
//
// NoScroll on both axes: the canvas fills the available space
// and scales its content to fit rather than adding scroll bars.
// ============================================================
class DenoisingViewScroller : public gui::ViewScroller
{
private:
    DenoisingView _canvas;

public:
    DenoisingViewScroller()
    : gui::ViewScroller(gui::ViewScroller::Type::NoScroll,
                        gui::ViewScroller::Type::NoScroll)
    {
        setContentView(&_canvas);
    }

    DenoisingView* getCanvas() { return &_canvas; }
};
