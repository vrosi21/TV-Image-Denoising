#pragma once
#include <gui/ToolBar.h>
#include <gui/Image.h>

// Two-button toolbar: Load (menuID=10, actionID=10)
//                     Denoise (menuID=20, actionID=10)
// Actions are dispatched to MainWindow::onActionItem().
class ControlsToolBar : public gui::ToolBar
{
    gui::Image _imgLoad;
    gui::Image _imgDenoise;
public:
    ControlsToolBar()
    : gui::ToolBar("mainTB", 2)
    , _imgLoad(":load")
    , _imgDenoise(":denoise")
    {
        reserve(2);
        addItem(tr("load"),    &_imgLoad,    tr("loadTT"),    10, 0, 0, 10);
        addItem(tr("denoise"), &_imgDenoise, tr("denoiseTT"), 20, 0, 0, 10);
    }
};
