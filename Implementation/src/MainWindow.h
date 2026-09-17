#pragma once
#include <gui/Window.h>
#include "MainMenuBar.h"
#include "MainToolBar.h"
#include "AppStatusBar.h"
#include "DenoisingPanel.h"
#include "WindowPlacement.h"

// ============================================================
// MainWindow: top-level application window
//
//   ┌ Menu:    App | File | Run | Help ─────────────────────────┐
//   ├ Toolbar: Images | Run  Lambda study  Stop | Export all  Open exports ┤
//   ├──────────────┬─────────────────────────────────────────────┤
//   │ Settings     │ Overview | Convergence | Lambda study |      │
//   │ sidebar      │ Report | Log                                 │
//   ├──────────────┴─────────────────────────────────────────────┤
//   └ Status:  image  ·  last result / progress message   [████] ┘
//
// SRP: window chrome (title, size, menu, toolbar, status) and
//      dispatch of menu/toolbar actions.  Logic is in DenoisingPanel.
// ============================================================
class MainWindow : public gui::Window
{
private:
    MainMenuBar    _menu;
    MainToolBar    _toolBar;
    AppStatusBar   _statusBar;
    DenoisingPanel _panel;

protected:
    bool onActionItem(gui::ActionItemDescriptor& aiDesc) override
    {
        auto [menuID, firstSubMenuID, lastSubMenuID, actionID] = aiDesc.getIDs();
        (void) firstSubMenuID; (void) lastSubMenuID;

        if (menuID == MainMenuBar::MenuFile)
        {
            switch (actionID)
            {
                case MainMenuBar::ActExportAll:     _panel.exportAll();                return true;
                case MainMenuBar::ActSaveImages:    _panel.saveDenoisedImages();       return true;
                case MainMenuBar::ActExportPdf:     _panel.exportCurrentChart(true);   return true;
                case MainMenuBar::ActExportSvg:     _panel.exportCurrentChart(false);  return true;
                case MainMenuBar::ActExportCsv:     _panel.exportData();               return true;
                case MainMenuBar::ActOpenExports:   _panel.openExportsFolder();        return true;
                case MainMenuBar::ActOpenImages:    _panel.openImagesFolder();         return true;
                case MainMenuBar::ActRefreshImages: _panel.refreshImages(true);        return true;
            }
        }
        else if (menuID == MainMenuBar::MenuRun)
        {
            switch (actionID)
            {
                case MainMenuBar::ActRun:   _panel.runComparison();  return true;
                case MainMenuBar::ActStudy: _panel.runLambdaStudy(); return true;
                case MainMenuBar::ActStop:  _panel.stop();           return true;
            }
        }
        else if (menuID == MainMenuBar::MenuHelp && actionID == MainMenuBar::ActAbout)
        {
            showAlert(td::String("TV Image Denoising"),
                      td::String("Total-variation image denoising: F(u) = 1/2 |u - f|^2 + lambda TV(u).\n\n"
                                 "Compares gradient descent with Armijo line search and the Newton method "
                                 "with Huber smoothing on identical noisy input.\n\n"
                                 "Numerical Optimisations, Faculty of Electrical Engineering Sarajevo."));
            return true;
        }
        return false;
    }

public:
    MainWindow()
    : gui::Window(placement::fitToScreen(1360, 860))
    {
        setTitle("TV Image Denoising");
        _menu.setAsMain(this);
        setToolBar(_toolBar);
        setStatusBar(_statusBar);

        _panel.setOnStatus([this](const td::String& s)    { _statusBar.setParams(s); });
        _panel.setOnProgress([this](double f)             { _statusBar.setProgress(f); });
        _panel.setOnImageName([this](const td::String& s) { _statusBar.setImageName(s); });
        setCentralView(&_panel);
    }

    bool shouldClose() override
    {
        _panel.cancelAndWait();
        return true;
    }
};
