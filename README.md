<div align="center">

# TV Image Denoising

**Total variation image denoising with gradient descent (Armijo backtracking) and a Newton-type lagged diffusivity method, in a cross-platform C++ desktop application built on natID.**

Numerical Optimisations · Data Science and Artificial Intelligence · Faculty of Electrical Engineering, University of Sarajevo

![C++](https://img.shields.io/badge/C++-20-blue)
![CMake](https://img.shields.io/badge/CMake-3.17+-green)
![natID](https://img.shields.io/badge/natID-framework-orange)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgray)

</div>

| | |
|---|---|
| **Course** | Numerical Optimisations |
| **Professor** | Izudin Džafić |
| **Student** | Kemal Sivro |
| **Student ID** | 20015 |
| **Academic year** | 2025/26 |
| **Paper** | [TV Image Denoising - Paper.pdf](docs/TV%20Image%20Denoising%20-%20Paper.pdf) |

## Overview

The application denoises a grayscale image $f$ by minimising

$$
F_\varepsilon(u) = \tfrac12 \lVert u - f \rVert_2^2 + \lambda \sum_{i,j} \sqrt{(D_x u)_{i,j}^2 + (D_y u)_{i,j}^2 + \varepsilon^2},
$$

a smoothed version of the Rudin-Osher-Fatemi model. The weight $\lambda$ controls how strongly noise is removed and $\varepsilon$ makes the functional twice differentiable, so both a first-order and a second-order method can be applied to it:

| Method | Iteration | Cost per iteration |
|---|---|---|
| Gradient descent | $u_{k+1} = \mathrm{clip}(u_k - t\,\nabla F_\varepsilon(u_k), 0, 1)$, $t$ from Armijo backtracking | $O(N)$ |
| Newton-type (lagged diffusivity, IRLS) | solve $(I + \lambda D^\top W(u_k) D)\,u_{k+1} = f$ with $w = 1/\sqrt{\lvert\nabla u_k\rvert^2 + \varepsilon^2}$ | dense LU up to 4096 pixels, sparse $LDL^\top$ above |

Both methods run on the same noisy image, generated from a seed so that the noise is identical on every platform. All plots use the exact TV energy ($\varepsilon = 0$) and the PSNR against the clean image, so the methods are measured on the same scale even though they use different smoothing.

## Features

- **Overview:** one card per method with PSNR, gain, iterations and time, marked best quality and fastest, above the original, noisy and denoised images.
- **Convergence:** exact energy gap, PSNR and relative change per iteration, and PSNR against wall time, for both methods on the same axes.
- **Lambda study:** PSNR against $\lambda$ with the best value per method, the L-curve and the computation cost.
- **Report and Log:** every number in tables, and a time-stamped session log.
- **Controls:** sliders for noise level, $\lambda$, $\varepsilon$ and iterations, a seed with a new-seed button, method selection and an option to re-run on every change. Computations run in the background and can be stopped.
- **Images:** Cameraman, Barbara and Ape are included. Your own PNG or JPEG files go into `Documents/TV Image Denoising/Images` (File, Open images folder) and appear after Refresh.
- **Export:** File, Export all writes charts (PDF), images (PNG) and data (CSV) into a new time-stamped folder under `Documents/TV Image Denoising/Exports`; Open exports folder shows it. No file dialogs are used.
- **Cross-platform:** Windows, macOS (Apple Silicon and Intel) and Linux from one code base.

Keyboard shortcuts (Ctrl on Windows and Linux, Cmd on macOS): R run, L lambda study, E export all, S save images, O open images folder.

## Installation

### Ready-made installers

Download the file for your platform from the [Releases](https://github.com/vrosi21/TV-Image-Denoising/releases) page.

| Platform | File | Installation |
|---|---|---|
| Windows 10/11 | `TVDenoising-win.zip` | Unzip and run the installer. Keep the `.exe` and `.msi` together. |
| macOS, Apple Silicon | `TVDenoising-macOS-Silicon.zip` | Unzip and move `TVDenoising.app` to Applications. See the note below. |
| macOS, Intel | `TVDenoising-macOS-Intel.zip` | As above. |
| Linux (Ubuntu 24.04 or newer) | `TVDenoising-linux.zip` | Unzip, then `sudo apt install ./TVDenoising*.deb`. |

> **macOS:** Safari unpacks downloaded archives and marks the app as quarantined. After moving the app to Applications run
> ```
> xattr -cr /Applications/TVDenoising.app
> ```
> If macOS still reports that the app is damaged, the quarantine flag was not removed.

### Build from source

Requirements: CMake 3.17 or newer, a C++20 compiler (MSVC 2022 or newer, Apple Clang 15+, GCC 12+) and the [natID SDK](https://github.com/idzafic/natID) in `$HOME/natID.SDK`. On Linux install `libgtk-4-dev` and `libadwaita-1-dev`.

```bash
git clone https://github.com/vrosi21/TV-Image-Denoising.git
cd TV-Image-Denoising/Implementation
mkdir -p ~/natID.RAMDisk/Out

# Linux and macOS
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Windows (Visual Studio)
cmake -B build -A x64
cmake --build build --config Release
```

natID places the executable in `~/natID.RAMDisk/Out/TVDenoising/Release/`, not in `build/`.

## Project structure

```
TV-Image-Denoising/
  Implementation/
    src/
      ITVSolver.h            solver interface with a per-iteration observer
      TVSolverGD.h           gradient descent with Armijo backtracking
      TVSolverNewton.h       lagged diffusivity method, dense LU or sparse LDLT
      ImageData.h            image buffer, PNG input and output, seeded noise
      DenoisingMetrics.h     exact TV energy, PSNR, relative change
      DenoisingRunner.h      comparison and lambda study without any GUI code
      DenoisingPanel.h       coordinates runs, results and exports
      ControlsPanel.h, ResultTabs.h, DenoisingView.h, ConvergenceView.h,
      LambdaStudyView.h, IterationLogView.h, LogView.h, Chart.h, ...
    res/                     test images, icons, translations
    CMakeLists.txt, TVDenoising.cmake
  docs/                      paper (PDF)
  packaging/                 installer configuration
  .github/workflows/         release pipeline for all platforms
  TVDenoising.desktop        Linux launcher
```

The solver classes depend only on `ITVSolver` and `ImageData`. The GUI talks to them through `DenoisingRunner`, which records metrics through the observer and supports cancellation, so a new method only needs a new `ITVSolver` implementation.

## Results in brief

On three 225 by 225 test images with noise level 0.1 both methods reach the same quality: at the best $\lambda$ their PSNR differs by at most 0.08 dB. Gradient descent costs 3.5 to 5.3 ms per iteration, the Newton-type method 101 to 123 ms, but the latter needs about ten times fewer iterations. The dense Newton system grows like $N^3$ and the sparse solver almost linearly. Details, formulas and all figures are in the [paper](docs/TV%20Image%20Denoising%20-%20Paper.pdf).

## License

MIT, see [LICENSE](LICENSE).
