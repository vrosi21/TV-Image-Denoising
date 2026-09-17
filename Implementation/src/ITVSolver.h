#pragma once
#include "ImageData.h"
#include <functional>
#include <vector>

// ============================================================
// ITVSolver: abstract interface for TV denoising solvers.
//
// convergenceHistory (optional):
//   If non-null, each solver appends F(u_k) after every
//   iteration so callers can analyse or plot convergence.
//   F(u) = ½‖u − f‖² + λ·TV_ε(u)
//
// observer (optional):
//   Called after every completed iteration with the iteration
//   number (1-based) and the current iterate u_k.  Returning
//   false stops the solver early (used for Stop / cancellation
//   and for recording quality metrics outside the solver).
// ============================================================
class ITVSolver
{
public:
    using IterationObserver = std::function<bool(int iteration, const ImageData& u)>;

    virtual ~ITVSolver() = default;

    virtual void denoise(const ImageData&         f,
                         ImageData&               u,
                         float                    lambda,
                         int                      maxIter,
                         std::vector<float>*      convergenceHistory = nullptr,
                         const IterationObserver* observer = nullptr) = 0;
};
