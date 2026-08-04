#pragma once
#include <gui/Application.h>
#include "MainWindow.h"

// ============================================================
// Application — NatID application entry point
// SRP: creates the first window and hands off control.
// ============================================================
class Application : public gui::Application
{
protected:
    gui::Window* createInitialWindow() override
    {
        auto pWnd = new MainWindow();
        setInitialFrameSize(gui::Window::FrameSize::Maximized);
        return pWnd;
    }

public:
    Application(int argc, const char** argv)
    : gui::Application(argc, argv)
    {}
};
