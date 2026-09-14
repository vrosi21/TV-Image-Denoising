#pragma once
#include <gui/Window.h>
#include "DenoisingPanel.h"

// ============================================================
// MainWindow — top-level application window
//
// Widget tree:
//   Window
//     └── setCentralView → DenoisingPanel (View)
//           SplitterLayout (Horizontal)
//             ├── DenoisingView  (Canvas) — Original | Denoised
//             └── RightPanel     (View)   — ControlsPanel + log
//
// SRP: manages window chrome (title, size) only.
//      All logic is in DenoisingPanel.
// ============================================================
class MainWindow : public gui::Window
{
private:
    DenoisingPanel _panel;

public:
    MainWindow()
    : gui::Window(gui::Size(1100, 700))
    {
        setTitle("TV Image Denoising — GD / Newton-Huber");
        setCentralView(&_panel);
    }

    bool shouldClose() override { return true; }
};
