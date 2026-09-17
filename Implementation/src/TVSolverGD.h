#pragma once
#include "ITVSolver.h"
#include <cmath>
#include <vector>
#include <cstdio>

// ============================================================
// TVSolverGD: Gradient Descent with Backtracking Line Search
//
// Minimises:  F(u) = ½‖u − f‖² + λ · TV_ε(u)
//
// TV_ε(u) = Σ_{i,j} √(dx_{ij}² + dy_{ij}² + ε²)
//   dx_{ij} = u(i,j+1) − u(i,j),  dy_{ij} = u(i+1,j) − u(i,j)
//
// Per outer iteration:
//   1. g  = ∇F(u) = (u − f) + λ · ∇TV_ε(u)
//   2. Armijo backtracking: find t s.t.
//        F(u − t·g) ≤ F(u) − α·t·‖g‖²
//   3. u ← clip(u − t·g, 0, 1)
//
// Convergence: linear (first-order). Serves as the baseline
// against which Newton's quadratic convergence is compared.
// ============================================================
class TVSolverGD : public ITVSolver
{
private:
    float _epsilon; // Huber smoothing (fixed for GD, not user-exposed)

    // F(u) = 0.5*||u-f||^2 + lambda * TV_eps(u)
    float computeF(const ImageData& u,
                   const ImageData& f,
                   float            lambda) const
    {
        int   N        = u.size();
        float fidelity = 0.f;
        for (int k = 0; k < N; ++k)
        {
            float d = u.pixels[k] - f.pixels[k];
            fidelity += d * d;
        }

        int W = u.width, H = u.height;
        float tv = 0.f;
        for (int i = 0; i < H; ++i)
            for (int j = 0; j < W; ++j)
            {
                float dx = (j+1 < W) ? u.at(i,j+1) - u.at(i,j) : 0.f;
                float dy = (i+1 < H) ? u.at(i+1,j) - u.at(i,j) : 0.f;
                tv += std::sqrt(dx*dx + dy*dy + _epsilon*_epsilon);
            }
        return 0.5f * fidelity + lambda * tv;
    }

    // g = (u-f) + lambda * grad_TV_eps(u)
    // grad_TV_eps is the negative discrete divergence of the normalised gradient
    void computeGradient(const ImageData&    u,
                         const ImageData&    f,
                         float               lambda,
                         std::vector<float>& g) const
    {
        int W = u.width, H = u.height, N = W * H;

        std::vector<float> dx(N,0.f), dy(N,0.f), w(N,0.f);
        for (int i = 0; i < H; ++i)
            for (int j = 0; j < W; ++j)
            {
                int k  = i*W + j;
                dx[k]  = (j+1 < W) ? u.at(i,j+1) - u.at(i,j) : 0.f;
                dy[k]  = (i+1 < H) ? u.at(i+1,j) - u.at(i,j) : 0.f;
                float d = std::sqrt(dx[k]*dx[k] + dy[k]*dy[k] + _epsilon*_epsilon);
                w[k]   = 1.f / d;
            }

        g.resize(N);
        for (int i = 0; i < H; ++i)
            for (int j = 0; j < W; ++j)
            {
                int   k     = i*W + j;
                float gTV   = 0.f;
                gTV -= w[k] * dx[k];                               // forward-x
                gTV -= w[k] * dy[k];                               // forward-y
                if (j > 0) gTV += w[i*W+j-1]   * dx[i*W+j-1];   // backward-x
                if (i > 0) gTV += w[(i-1)*W+j] * dy[(i-1)*W+j]; // backward-y
                g[k] = (u.pixels[k] - f.pixels[k]) + lambda * gTV;
            }
    }

public:
    explicit TVSolverGD(float epsilon = 1e-3f)
    : _epsilon(epsilon)
    {}

    void denoise(const ImageData&         f,
                 ImageData&               u,
                 float                    lambda,
                 int                      maxIter,
                 std::vector<float>*      history = nullptr,
                 const IterationObserver* observer = nullptr) override
    {
        u = f;
        if (history) history->clear();

        const float alpha = 0.01f;
        const float beta  = 0.5f;
        const int   maxBT = 40;

        std::vector<float> g;
        ImageData u_trial(f.width, f.height);

        for (int iter = 0; iter < maxIter; ++iter)
        {
            computeGradient(u, f, lambda, g);

            float gnorm2 = 0.f;
            for (float gi : g) gnorm2 += gi * gi;

            float Fu = computeF(u, f, lambda);
            if (history) history->push_back(Fu);
            std::printf("  GD  iter %2d  F=%.6f\n", iter + 1, static_cast<double>(Fu));

            if (gnorm2 < 1e-10f) break;

            float t = 1.0f;
            for (int bt = 0; bt < maxBT; ++bt)
            {
                int N = u.size();
                for (int k = 0; k < N; ++k)
                    u_trial.pixels[k] = u.pixels[k] - t * g[k];

                if (computeF(u_trial, f, lambda) <= Fu - alpha * t * gnorm2)
                    break;
                t *= beta;
            }

            int N = u.size();
            for (int k = 0; k < N; ++k)
            {
                float v = u_trial.pixels[k];
                u.pixels[k] = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
            }

            if (observer && !(*observer)(iter + 1, u))
                break;
        }
    }
};
