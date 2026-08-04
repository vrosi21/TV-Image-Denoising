// TV Image Denoising — Numerical Optimization Project
// University of Sarajevo, Faculty of Electrical Engineering
//
// Entry point: creates the NatID GUI application and the initial window.

#include "Application.h"
#include <gui/WinMain.h>

int main(int argc, const char* argv[])
{
    Application app(argc, argv);
    app.init("EN");
    return app.run();
}
