#pragma once
#include "ITVSolver.h"
#include <dense/Matrix.h>          // NatID dense matrix — used when N is small
#include <sparse/ISolver.h>        // NatID sparse LDLT — fallback for large images
#include <mem/PointerReleaser.h>
#include <cmath>
#include <cstdio>
#include <vector>

// ============================================================
// TVSolverNewton — Newton's Method with Huber TV regularisation
//
// Minimises:  F(u) = ½‖u − f‖² + λ · TV_ε(u)
//
// The Huber smoothing makes F twice differentiable:
//   TV_ε(u) = Σ_{i,j} √(dx² + dy² + ε²)
//
// Hessian:  H = I + λ · D^T W D
//   W diagonal: w_{ij} = 1/√(dx²+dy²+ε²)
//
// Newton step per iteration:  solve  H · u_new = f
//   (this equals IRLS — the IRLS update IS a Newton step)
//
// ── Linear solver strategy ──────────────────────────────────
// For images with N ≤ kMaxDenseN pixels the Newton system is
// assembled and solved as a full dense::DblMatrix, exactly as
// required by the project spec (Lab 6 dense matrix API).
//
// For larger images (225×225 → N≈50 K) a dense N×N matrix
// would require ~20 GB of RAM — physically impossible.
// The sparse LDLT path is used automatically in that case,
// producing identical results with a fraction of the memory.
// The report should note this practical limitation.
//
// ε (user slider) governs the Huber approximation quality:
//   smaller ε → closer to true TV, sharper edges
//   larger  ε → smoother result, fewer iterations needed
//
// SRP: only the Newton/IRLS math lives here.
// ============================================================
class TVSolverNewton : public ITVSolver
{
public:
    // Images up to this many pixels use the dense path (≈64×64).
    static constexpr int kMaxDenseN = 4096;

private:
    float _epsilon;

    // ── Shared helpers ─────────────────────────────────────
    void computeWeights(const ImageData&    u,
                        std::vector<float>& wx,
                        std::vector<float>& wy) const
    {
        int W = u.width, H = u.height;
        wx.resize(W*H); wy.resize(W*H);
        for (int i = 0; i < H; ++i)
            for (int j = 0; j < W; ++j)
            {
                int   k  = i*W + j;
                float dx = (j+1<W) ? u.at(i,j+1)-u.at(i,j) : 0.f;
                float dy = (i+1<H) ? u.at(i+1,j)-u.at(i,j) : 0.f;
                float d  = std::sqrt(dx*dx + dy*dy + _epsilon*_epsilon);
                wx[k] = wy[k] = 1.f / d;
            }
    }

    float computeF(const ImageData& u,
                   const ImageData& f,
                   float            lambda) const
    {
        int N = u.size(); float fid = 0.f;
        for (int k = 0; k < N; ++k) { float d=u.pixels[k]-f.pixels[k]; fid+=d*d; }

        int W=u.width, H=u.height; float tv=0.f;
        for (int i=0;i<H;++i)
            for (int j=0;j<W;++j)
            {
                float dx=(j+1<W)?u.at(i,j+1)-u.at(i,j):0.f;
                float dy=(i+1<H)?u.at(i+1,j)-u.at(i,j):0.f;
                tv += std::sqrt(dx*dx+dy*dy+_epsilon*_epsilon);
            }
        return 0.5f*fid + lambda*tv;
    }

    // ── Dense solve path — used when N ≤ kMaxDenseN ────────
    // Assembles the full N×N Hessian H and RHS b using
    // NatID's dense::DblMatrix, then solves H·x = b in place.
    void solveNewtonDense(const ImageData&          f,
                          const std::vector<float>& wx,
                          const std::vector<float>& wy,
                          float                     lambda,
                          ImageData&                u) const
    {
        int W = f.width, H = f.height, N = W*H;

        dense::DblMatrix Hmat(static_cast<td::UINT4>(N),
                              static_cast<td::UINT4>(N));
        dense::DblMatrix bvec(static_cast<td::UINT4>(N), 1u);

        Hmat.zeros(); // explicit zero-init
        auto Hi = Hmat.getManipulator();
        auto bi = bvec.getFirstColumnManipulator();

        for (int i = 0; i < H; ++i)
        {
            for (int j = 0; j < W; ++j)
            {
                int k = i*W + j;

                float wR = (j+1<W) ? wx[k]           : 0.f;
                float wL = (j  >0) ? wx[i*W+j-1]     : 0.f;
                float wB = (i+1<H) ? wy[k]           : 0.f;
                float wA = (i  >0) ? wy[(i-1)*W+j]   : 0.f;

                // Diagonal
                Hi(k, k) = 1.0 + lambda*(wR+wL+wB+wA);

                // Off-diagonals (symmetric — set both sides)
                if (j > 0)
                {
                    int L = i*W+j-1;
                    double v = -lambda * static_cast<double>(wx[L]);
                    Hi(k, L) = v;
                    Hi(L, k) = v;
                }
                if (i > 0)
                {
                    int A = (i-1)*W+j;
                    double v = -lambda * static_cast<double>(wy[A]);
                    Hi(k, A) = v;
                    Hi(A, k) = v;
                }

                bi(k) = static_cast<double>(f.pixels[k]);
            }
        }

        if (!Hmat.solve(bvec)) { u = f; return; }

        u.width = W; u.height = H; u.pixels.resize(N);
        auto xi = bvec.getFirstColumnManipulator();
        for (int k = 0; k < N; ++k)
        {
            float v = static_cast<float>(xi(k));
            u.pixels[k] = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
        }
    }

    // ── Sparse LDLT path — fallback for large images ────────
    void solveNewtonSparse(const ImageData&          f,
                           const std::vector<float>& wx,
                           const std::vector<float>& wy,
                           float                     lambda,
                           ImageData&                u) const
    {
        int W = f.width, H = f.height, N = W*H;
        int nzEst = N + (N-H) + (N-W) + 10;

        sparse::DblSolverReleaser pSolver(
            sparse::createDblSolver(N, nzEst,
                                    sparse::Symmetry::SymmetricPosDef,
                                    sparse::SolverType::LDLT,
                                    sparse::Pivoting::No,
                                    sparse::Ordering::Own));
        sparse::DblSolver& s = pSolver.ref();

        for (int i = 0; i < H; ++i)
            for (int j = 0; j < W; ++j)
            {
                int k = i*W + j;
                float wR=(j+1<W)?wx[k]:0.f, wL=(j>0)?wx[i*W+j-1]:0.f;
                float wB=(i+1<H)?wy[k]:0.f, wA=(i>0)?wy[(i-1)*W+j]:0.f;
                s.addTriple(k, k, 1.0+lambda*(wR+wL+wB+wA));
                if (j>0) s.addTriple(k, i*W+j-1, -lambda*wx[i*W+j-1]);
                if (i>0) s.addTriple(k, (i-1)*W+j, -lambda*wy[(i-1)*W+j]);
                s.setRHS(k, static_cast<double>(f.pixels[k]));
            }

        if (!s.factorize()) { u = f; return; }
        if (!s.solve())     { u = f; return; }

        u.width = W; u.height = H; u.pixels.resize(N);
        for (int k = 0; k < N; ++k)
        {
            float v = static_cast<float>(s.x(k));
            u.pixels[k] = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
        }
    }

public:
    explicit TVSolverNewton(float epsilon = 1e-2f)
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

        const bool useDense = (f.size() <= kMaxDenseN);
        if (useDense)
            std::printf("  Newton: using dense::DblMatrix path (N=%d)\n", f.size());
        else
            std::printf("  Newton: using sparse LDLT fallback (N=%d > %d)\n",
                        f.size(), kMaxDenseN);

        std::vector<float> wx, wy;

        for (int iter = 0; iter < maxIter; ++iter)
        {
            computeWeights(u, wx, wy);

            if (useDense) solveNewtonDense (f, wx, wy, lambda, u);
            else          solveNewtonSparse(f, wx, wy, lambda, u);

            float Fu = computeF(u, f, lambda);
            if (history) history->push_back(Fu);
            std::printf("  Newton iter %2d  F=%.6f\n", iter+1,
                        static_cast<double>(Fu));

            if (observer && !(*observer)(iter + 1, u))
                break;
        }
    }
};
