#pragma once
#include <gui/ToolBar.h>
#include <gui/Image.h>
#include "MainMenuBar.h"

// ============================================================
// MainToolBar — primary actions, left to right in workflow order:
//   Images | Run, Lambda study, Stop | Export all, Open exports
// Items reuse the menu IDs, so one handler serves menu and toolbar.
// Icons are registered in res/main.xml.
// ============================================================
class MainToolBar : public gui::ToolBar
{
    gui::Image _imgImages;
    gui::Image _imgRun;
    gui::Image _imgStudy;
    gui::Image _imgStop;
    gui::Image _imgExport;
    gui::Image _imgExportsFolder;

public:
    MainToolBar()
    : gui::ToolBar("tvdenMainTB", 8)
    , _imgImages(":tbImages")
    , _imgRun(":tbRun")
    , _imgStudy(":tbStudy")
    , _imgStop(":tbStop")
    , _imgExport(":tbExport")
    , _imgExportsFolder(":tbExportsFolder")
    {
        using M = MainMenuBar;
        addItem("Images", &_imgImages, "Open the images folder: PNG/JPEG files there appear in the image list", M::MenuFile, 0, 0, M::ActOpenImages);
        addSpaceItem();
        addItem("Run", &_imgRun, "Denoise with the selected methods and compare them (Ctrl+R)", M::MenuRun, 0, 0, M::ActRun);
        addItem("Lambda study", &_imgStudy, "Solve for a range of lambda values with each method (Ctrl+L)", M::MenuRun, 0, 0, M::ActStudy);
        addItem("Stop", &_imgStop, "Cancel the running computation", M::MenuRun, 0, 0, M::ActStop);
        addSpaceItem();
        addItem("Export all", &_imgExport, "Charts (PDF), images (PNG) and data (CSV) into a new dated folder (Ctrl+E)", M::MenuFile, 0, 0, M::ActExportAll);
        addItem("Open exports", &_imgExportsFolder, "Open the folder of the latest export", M::MenuFile, 0, 0, M::ActOpenExports);
    }
};
