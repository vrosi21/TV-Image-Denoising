#pragma once
#include <gui/MenuBar.h>

// ============================================================
// MainMenuBar: application menus.  The toolbar uses the same
// menu/action IDs, so MainWindow::onActionItem() handles both.
// Exports go to a dated folder under Documents/TV Image Denoising/
// Exports (no file dialogs).
// ============================================================
class MainMenuBar : public gui::MenuBar
{
    gui::SubMenu _app;
    gui::SubMenu _file;
    gui::SubMenu _run;
    gui::SubMenu _help;

public:
    enum Menu : unsigned char { MenuApp = 10, MenuFile = 20, MenuRun = 30, MenuHelp = 40 };
    enum FileAction : unsigned char { ActExportAll = 10, ActSaveImages = 20, ActExportPdf = 30, ActExportSvg = 40,
                                      ActExportCsv = 50, ActOpenExports = 60, ActOpenImages = 70, ActRefreshImages = 80 };
    enum RunAction : unsigned char { ActRun = 10, ActStudy = 20, ActStop = 30 };
    enum HelpAction : unsigned char { ActAbout = 10 };

    MainMenuBar()
    : gui::MenuBar(4)
    , _app(MenuApp, "App", 1)
    , _file(MenuFile, "File", 10)
    , _run(MenuRun, "Run", 4)
    , _help(MenuHelp, "Help", 1)
    {
        _app.getItems()[0].initAsQuitAppActionItem(tr("Quit"), "q");

        auto& file = _file.getItems();
        file[0].initAsActionItem("Export all (charts, images, data)", ActExportAll, "e");
        file[1].initAsActionItem("Export current chart (PDF)", ActExportPdf);
        file[2].initAsActionItem("Export current chart (SVG)", ActExportSvg);
        file[3].initAsActionItem("Save images (PNG)", ActSaveImages, "s");
        file[4].initAsActionItem("Export data (CSV)", ActExportCsv);
        file[5].initAsSeparator();
        file[6].initAsActionItem("Open exports folder", ActOpenExports);
        file[7].initAsSeparator();
        file[8].initAsActionItem("Open images folder", ActOpenImages, "o");
        file[9].initAsActionItem("Refresh image list", ActRefreshImages);

        auto& run = _run.getItems();
        run[0].initAsActionItem("Run comparison", ActRun, "r");
        run[1].initAsActionItem("Run lambda study", ActStudy, "l");
        run[2].initAsSeparator();
        run[3].initAsActionItem("Stop", ActStop);

        _help.getItems()[0].initAsActionItem("About TV Image Denoising", ActAbout);

        setMenu(0, &_app);
        setMenu(1, &_file);
        setMenu(2, &_run);
        setMenu(3, &_help);
    }
};
