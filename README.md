<div align="center">

# TV Image Denoising

**Total-variation image denoising with an interactive GUI — Gradient Descent (Armijo backtracking) and Newton's Method (Huber smoothing), built with C++ and the natID framework.**

**Academic Project** • Numerical Optimisations • Data Science and AI • ETF Sarajevo

![C++](https://img.shields.io/badge/C++-20-blue)
![CMake](https://img.shields.io/badge/CMake-3.17+-green)
![natID](https://img.shields.io/badge/natID-Framework-orange)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgray)

</div>

---

## Overview

TV Image Denoising solves the total-variation minimisation problem

> **F(u) = ½‖u − f‖² + λ · TVε(u)**

where **f** is the noisy input image and **λ** controls the denoising strength. The Huber-smoothed TV term keeps the gradient Lipschitz-continuous, enabling both first- and second-order solvers.

**Academic context:**

| | |
|---|---|
| **Course** | Numerical Optimisations |
| **Professor** | Prof. Dr. Izudin Džafić |
| **Student** | Kemal Sivro |
| **Student ID** | 20015 |
| **Academic Year** | 2025/26 |

---

## Features

- **Two solvers** — Gradient Descent with Armijo backtracking line-search, and Newton's Method via IRLS (Huber)
- **Interactive GUI** — load any image, adjust λ and ε live, run/stop at any point
- **Convergence plot** — real-time F(u) vs. iteration chart for both solvers
- **Side-by-side view** — original and denoised images displayed together
- **Test images included** — Cameraman, Barbara, Ape (standard denoising benchmarks)
- **Cross-platform** — Windows, macOS (Intel + Apple Silicon), Linux

---

## Installation

### Option A — Download a ready-made installer (recommended)

Grab an installer for your platform from the [**Releases**](https://github.com/vrosi21/TV-Image-Denoising/releases) page — no build tools required.

| Platform | File | How to install |
|---|---|---|
| 🪟 **Windows 10/11** | `TVDenoising-win.zip` | Unzip, run the `.exe` (or `.msi` directly). Keep both files together. |
| 🍎 **macOS Apple Silicon** (M1–M4) | `TVDenoising-macOS-Silicon.zip` | Unzip, drag `TVDenoising.app` to Applications. See macOS note below. |
| 🍎 **macOS Intel** (2016–2020) | `TVDenoising-macOS-Intel.zip` | Same as above. |
| 🐧 **Linux** (Ubuntu 24.04+) | `TVDenoising-linux.zip` | Unzip, then `sudo apt install ./TVDenoising*.deb` |

> ⚠️ **macOS quarantine note:** Safari auto-extracts zips on download, re-applying the quarantine flag. After moving `TVDenoising.app` to Applications, run:
> ```
> xattr -cr /Applications/TVDenoising.app
> ```
> No output = success. If you see "damaged or incomplete", the quarantine flag wasn't cleared — run the command above.

### Option B — Build from source

#### Prerequisites

- CMake 3.17+
- C++20 compiler (MSVC 2022, Apple Clang 15+, GCC 12+)
- [natID SDK](https://github.com/idzafic/natID) installed at `$HOME/natID.SDK`

#### Build

```bash
git clone https://github.com/vrosi21/TV-Image-Denoising.git
cd TV-Image-Denoising/Implementation

mkdir -p ~/natID.RAMDisk/Out   # required by natID DevEnv

# Linux / macOS
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Windows (Visual Studio 2022)
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

---

## Project Structure

```
TV-Image-Denoising/
  Implementation/          # C++ source & CMake
    src/                   # Application, solvers, GUI components
    res/                   # Images, icons, translations
    CMakeLists.txt
    TVDenoising.cmake
    TVDenoising.desktop    # Linux launcher
  docs/                    # Project report (PDF)
  .github/workflows/       # CI — builds installers on tag push
  LICENSE
  README.md
```

---

## Algorithm Summary

| Solver | Method | Line search |
|---|---|---|
| GD | Gradient descent | Armijo backtracking |
| Newton | IRLS (Huber-smoothed Hessian) | Fixed step (ε controls smoothing) |

The Huber TV functional replaces `|∇u|` with a smooth approximation for `|∇u| < ε`, making the Hessian positive-definite and Newton's method applicable.

---

## License

MIT — see [LICENSE](LICENSE).
