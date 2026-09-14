#pragma once
#include <gui/View.h>
#include <gui/SplitterLayout.h>
#include <gui/Types.h>          // gui::getResFileName
#include "DenoisingView.h"
#include "RightPanel.h"
#include "ImageData.h"
#include "TVSolverGD.h"
#include "TVSolverNewton.h"
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <vector>

// ============================================================
// DenoisingPanel — top-level coordinator view
//
// Widget tree:
//   SplitterLayout (Horizontal, AuxiliaryCell::Second)
//     ├── DenoisingView  (Canvas, main/left)  — Original | Denoised
//     └── RightPanel     (View,   aux/right)
//           ├── ControlsPanel  (top) — algorithm, image, sliders, button
//           └── TextEdit log   (bottom) — convergence output per run
//
// Image paths are resolved at runtime via gui::getResFileName(),
// registered in res/main.xml — no hardcoded or build-time paths.
//
// SRP: coordinates image loading, noise generation, and solver
//      dispatch only.  Rendering → DenoisingView.
//      Control layout → ControlsPanel (via RightPanel).
//      Log display    → RightPanel.
// ============================================================
class DenoisingPanel : public gui::View
{
private:
    gui::SplitterLayout _sl;
    DenoisingView       _canvas;
    RightPanel          _right;

    ImageData _original;
    ImageData _denoised;
    int       _currentImage = -1;

    // Resource keys matching <FileNames> entries in res/main.xml
    static constexpr const char* kImageKeys[3] = {
        "cameraman", "ape", "barbara"
    };

    // ----------------------------------------------------------
    // Log helper — formats a line and appends it to the log view
    // ----------------------------------------------------------
    void log(const char* fmt, ...)
#ifdef __GNUC__
        __attribute__((format(printf, 2, 3)))
#endif
    {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        _right.appendLog(buf);
    }

    // ----------------------------------------------------------
    void runDenoising()
    {
        if (_currentImage < 0)
        {
            showAlert(td::String("No image"),
                      td::String("Please select an image first."));
            return;
        }

        float sigma  = _right.getNoiseSigma();
        float lambda = _right.getLambda();
        int   iters  = _right.getIterations();
        float eps    = _right.getEpsilon();
        int   algo   = _right.getAlgorithm();

        _right.clearLog();

        const char* algoName = (algo == 0) ? "GD Backtracking" : "Newton-Huber";
        log("Algorithm : %s\n", algoName);
        log("lambda    : %.4f\n", static_cast<double>(lambda));
        log("sigma     : %.4f\n", static_cast<double>(sigma));
        log("iterations: %d\n",   iters);
        if (algo == 1)
            log("epsilon   : %.4f\n", static_cast<double>(eps));
        log("-----------------------------\n");

        try
        {
            ImageData noisy = _original;
            noisy.addGaussianNoise(sigma); // seeds internally via <random>

            std::vector<float> history;

            if (algo == 0)
            {
                TVSolverGD solver;
                solver.denoise(noisy, _denoised, lambda, iters, &history);
            }
            else
            {
                int N = noisy.size();
                if (N <= TVSolverNewton::kMaxDenseN)
                    log("Solver    : dense::DblMatrix (N=%d)\n", N);
                else
                    log("Solver    : sparse LDLT (N=%d > %d)\n",
                        N, TVSolverNewton::kMaxDenseN);
                log("-----------------------------\n");

                TVSolverNewton solver(eps);
                solver.denoise(noisy, _denoised, lambda, iters, &history);
            }

            log("Iter   F(u)\n");
            for (int i = 0; i < static_cast<int>(history.size()); ++i)
                log("%3d    %.6f\n", i + 1, static_cast<double>(history[i]));

            log("-----------------------------\n");
            if (!history.empty())
                log("Total drop: %.6f\n",
                    static_cast<double>(history.front() - history.back()));

            _canvas.setDenoised(_denoised);
        }
        catch (const std::exception& ex)
        {
            log("[ERROR] %s\n", ex.what());
            showAlert(td::String("Denoising error"), td::String(ex.what()));
        }
        catch (...)
        {
            log("[ERROR] Unknown exception.\n");
            showAlert(td::String("Denoising error"),
                      td::String("An unexpected error occurred."));
        }
    }

    // ----------------------------------------------------------
    // Shared image-loading core — accepts any resolved path.
    // ----------------------------------------------------------
    void loadImageFromPath(const char* path)
    {
        if (!_original.loadFromFile(path))
        {
            td::String msg("Cannot open: ");
            msg += td::String(path);
            showAlert(td::String("Load error"), msg);
            return;
        }
        _canvas.setOriginalFromFile(path);
        _canvas.clearDenoised();
        _currentImage = 0; // marks "some image is loaded"
    }

public:
    // Load one of the three built-in test images by index.
    void loadImage(int idx)
    {
        if (idx < 0 || idx > 2) return;
        td::String path = gui::getResFileName(kImageKeys[idx]);
        loadImageFromPath(path.c_str());
    }

    // Load a user-chosen image from an arbitrary file-system path.
    void loadCustomImage(const char* path)
    {
        loadImageFromPath(path);
    }

    DenoisingPanel()
    : _sl(gui::SplitterLayout::Orientation::Horizontal,
          gui::SplitterLayout::AuxiliaryCell::Second)
    {
        _right.setOnSelectImage([this](int idx)      { loadImage(idx);       });
        _right.setOnBrowse([this](const char* path)  { loadCustomImage(path); });
        _right.setOnRun([this]()                     { runDenoising();        });

        setMargins(0, 0, 0, 0);
        _sl.setMargins(0, 0);
        _sl.setContent(_canvas, _right);
        setLayout(&_sl);
    }

    void onInitialAppearance() override
    {
        loadImage(0);
    }
};
