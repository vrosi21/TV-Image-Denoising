#pragma once
#include "ImageData.h"
#include <algorithm>
#include <cmath>

// ============================================================
// DenoisingMetrics: solver-independent quality measures.
//
// The solvers use different Huber parameters (GD: fixed 1e-3,
// Newton: user epsilon), so their internal F values are not
// directly comparable.  Every plot therefore uses the exact
// (epsilon = 0) isotropic TV energy and the PSNR against the
// clean original, computed here for any iterate u.
//
// SRP: pure functions of images, no GUI, no solver state.
// ============================================================
namespace metrics
{

// ½‖u − f‖²
inline double fidelity(const ImageData& u, const ImageData& f)
{
    double s = 0;
    for (int k = 0; k < u.size(); ++k)
    {
        const double d = u.pixels[k] - f.pixels[k];
        s += d * d;
    }
    return 0.5 * s;
}

// TV(u) = Σ √(dx² + dy²)  with forward differences (same stencil as the solvers)
inline double totalVariation(const ImageData& u)
{
    double tv = 0;
    for (int i = 0; i < u.height; ++i)
        for (int j = 0; j < u.width; ++j)
        {
            const double dx = (j + 1 < u.width)  ? u.at(i, j + 1) - u.at(i, j) : 0.0;
            const double dy = (i + 1 < u.height) ? u.at(i + 1, j) - u.at(i, j) : 0.0;
            tv += std::sqrt(dx * dx + dy * dy);
        }
    return tv;
}

// F(u) = ½‖u − f‖² + λ TV(u)
inline double energy(const ImageData& u, const ImageData& f, double lambda)
{
    return fidelity(u, f) + lambda * totalVariation(u);
}

// Peak signal-to-noise ratio in dB for images in [0, 1]
inline double psnr(const ImageData& u, const ImageData& reference)
{
    double mse = 0;
    for (int k = 0; k < u.size(); ++k)
    {
        const double d = u.pixels[k] - reference.pixels[k];
        mse += d * d;
    }
    mse /= (std::max)(1, u.size());
    if (mse <= 1e-12) return 120.0;
    return 10.0 * std::log10(1.0 / mse);
}

// ‖a − b‖ / ‖b‖
inline double relativeChange(const ImageData& a, const ImageData& b)
{
    double num = 0, den = 0;
    for (int k = 0; k < a.size(); ++k)
    {
        const double d = a.pixels[k] - b.pixels[k];
        num += d * d;
        den += static_cast<double>(b.pixels[k]) * b.pixels[k];
    }
    return std::sqrt(num) / (std::max)(std::sqrt(den), 1e-30);
}

} // namespace metrics
