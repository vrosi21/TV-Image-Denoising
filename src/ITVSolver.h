#pragma once
#include "ImageData.h"
#include <vector>

// ============================================================
// ITVSolver — abstract interface for TV denoising solvers.
//
// convergenceHistory (optional):
//   If non-null, each solver appends F(u_k) after every
//   iteration so callers can analyse or plot convergence.
//   F(u) = ½‖u − f‖² + λ·TV_ε(u)
// ============================================================
class ITVSolver
{
public:
    virtual ~ITVSolver() = default;

    virtual void denoise(const ImageData&    f,
                         ImageData&          u,
                         float               lambda,
                         int                 maxIter,
                         std::vector<float>* convergenceHistory = nullptr) = 0;
};
