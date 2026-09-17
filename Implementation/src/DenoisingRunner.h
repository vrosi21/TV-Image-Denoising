#pragma once
#include "ImageData.h"
#include "DenoisingMetrics.h"
#include "TVSolverGD.h"
#include "TVSolverNewton.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

// ============================================================
// DenoisingRunner: executes one experiment, GUI-free.
//
//   Comparison  : every enabled solver on the same noisy image
//   LambdaStudy : every enabled solver for log-spaced lambda
//                 values: PSNR, L-curve and cost per method
//
// Noise is generated once from a seed, so all solvers and all
// lambda values see identical input.  Per-iteration metrics are
// collected through ITVSolver's observer, which also delivers
// cancellation and progress.  Runs on a worker thread; the result
// is an immutable value handed to the GUI.
// ============================================================

enum class SolverKind { GD = 0, Newton = 1 };
enum class RunMode { Comparison, LambdaStudy };

inline const char* solverName(SolverKind k)
{
    return k == SolverKind::GD ? "Gradient descent" : "Newton (Huber)";
}

inline const char* solverShortName(SolverKind k)
{
    return k == SolverKind::GD ? "GD" : "Newton";
}

struct DenoiseSettings
{
    bool     runGD      = true;
    bool     runNewton  = true;
    float    lambda     = 0.10f;
    float    sigma      = 0.10f;
    float    epsilon    = 0.01f;
    int      iterations = 10;
    unsigned seed       = 12345;
    double   studyFrom  = 0.01;
    double   studyTo    = 1.0;
    int      studyPoints = 10;

    std::vector<SolverKind> solvers() const
    {
        std::vector<SolverKind> k;
        if (runGD) k.push_back(SolverKind::GD);
        if (runNewton) k.push_back(SolverKind::Newton);
        return k;
    }
};

struct IterationRecord
{
    int    k = 0;            // 0 = noisy input
    double energy = 0;       // ½‖u − f‖² + λ TV(u)   (exact TV)
    double psnr = 0;         // dB against the clean original
    double relChange = 0;    // ‖u_k − u_{k−1}‖ / ‖u_{k−1}‖
    double timeMs = 0;       // wall time since the solver started
};

struct SolverRun
{
    SolverKind kind = SolverKind::GD;
    ImageData  result;
    std::vector<IterationRecord> iterations;
    double totalMs = 0;

    const IterationRecord& last() const { return iterations.back(); }
};

struct StudyPoint
{
    double lambda = 0;
    double psnr = 0;
    double fidelity = 0;     // ½‖u − f‖²
    double tv = 0;           // TV(u)
    double timeMs = 0;
};

struct StudySeries
{
    SolverKind kind = SolverKind::GD;
    std::vector<StudyPoint> points;

    int bestIndex() const
    {
        int best = -1;
        for (int i = 0; i < (int) points.size(); ++i)
            if (best < 0 || points[i].psnr > points[best].psnr) best = i;
        return best;
    }
};

struct DenoiseResult
{
    RunMode         mode = RunMode::Comparison;
    DenoiseSettings settings;
    std::string     imageName;
    ImageData       original;
    ImageData       noisy;
    double          noisyPsnr = 0;
    std::vector<SolverRun>   runs;     // Comparison
    std::vector<StudySeries> study;    // LambdaStudy
    bool            cancelled = false;

    const SolverRun* run(SolverKind k) const
    {
        for (auto& r : runs) if (r.kind == k) return &r;
        return nullptr;
    }
};

using DenoiseResultPtr = std::shared_ptr<const DenoiseResult>;

class DenoisingRunner
{
public:
    using Progress = std::function<void(const std::string& stage, double fraction)>;

    static std::unique_ptr<ITVSolver> makeSolver(SolverKind kind, float epsilon)
    {
        if (kind == SolverKind::GD) return std::make_unique<TVSolverGD>();
        return std::make_unique<TVSolverNewton>(epsilon);
    }

    static ImageData makeNoisy(const ImageData& original, float sigma, unsigned seed)
    {
        ImageData noisy = original;
        noisy.addGaussianNoise(sigma, seed);
        return noisy;
    }

    static double studyLambda(const DenoiseSettings& s, int i)
    {
        const int n = (std::max)(2, s.studyPoints);
        const double t = static_cast<double>(i) / (n - 1);
        return s.studyFrom * std::pow(s.studyTo / s.studyFrom, t);
    }

    static DenoiseResult run(const ImageData& original, const std::string& imageName,
                             const DenoiseSettings& s, RunMode mode,
                             const std::atomic<bool>& cancel, const Progress& progress)
    {
        DenoiseResult res;
        res.mode = mode;
        res.settings = s;
        res.imageName = imageName;
        res.original = original;
        res.noisy = makeNoisy(original, s.sigma, s.seed);
        res.noisyPsnr = metrics::psnr(res.noisy, original);

        const std::vector<SolverKind> kinds = s.solvers();
        const int nSolvers = (std::max)(1, (int) kinds.size());

        if (mode == RunMode::Comparison)
        {
            for (int i = 0; i < (int) kinds.size() && !cancel.load(); ++i)
            {
                auto stage = [&](double frac)
                {
                    if (progress) progress(solverName(kinds[i]), (i + frac) / nSolvers);
                };
                res.runs.push_back(solve(res, kinds[i], s, cancel, stage));
            }
        }
        else
        {
            const int n = (std::max)(2, s.studyPoints);
            for (int m = 0; m < (int) kinds.size(); ++m)
            {
                StudySeries series;
                series.kind = kinds[m];
                for (int i = 0; i < n && !cancel.load(); ++i)
                {
                    DenoiseSettings si = s;
                    si.lambda = static_cast<float>(studyLambda(s, i));
                    auto stage = [&](double frac)
                    {
                        if (!progress) return;
                        char buf[96];
                        std::snprintf(buf, sizeof buf, "%s, lambda %d of %d", solverName(kinds[m]), i + 1, n);
                        progress(buf, (m + (i + frac) / n) / nSolvers);
                    };
                    SolverRun r = solve(res, kinds[m], si, cancel, stage);
                    if (cancel.load()) break;

                    StudyPoint p;
                    p.lambda = si.lambda;
                    p.psnr = r.last().psnr;
                    p.fidelity = metrics::fidelity(r.result, res.noisy);
                    p.tv = metrics::totalVariation(r.result);
                    p.timeMs = r.totalMs;
                    series.points.push_back(p);
                }
                res.study.push_back(series);
                if (cancel.load()) break;
            }
        }

        res.cancelled = cancel.load();
        return res;
    }

private:
    static SolverRun solve(const DenoiseResult& res, SolverKind kind, const DenoiseSettings& s,
                           const std::atomic<bool>& cancel, const std::function<void(double)>& stage)
    {
        using clock = std::chrono::steady_clock;
        SolverRun run;
        run.kind = kind;

        IterationRecord r0;
        r0.energy = metrics::energy(res.noisy, res.noisy, s.lambda);
        r0.psnr = res.noisyPsnr;
        run.iterations.push_back(r0);

        ImageData previous = res.noisy;
        const auto t0 = clock::now();
        const int total = (std::max)(1, s.iterations);

        ITVSolver::IterationObserver observer = [&](int k, const ImageData& u)
        {
            IterationRecord rec;
            rec.k = k;
            rec.timeMs = std::chrono::duration<double, std::milli>(clock::now() - t0).count();
            rec.energy = metrics::energy(u, res.noisy, s.lambda);
            rec.psnr = metrics::psnr(u, res.original);
            rec.relChange = metrics::relativeChange(u, previous);
            run.iterations.push_back(rec);
            previous = u;
            if (stage) stage(static_cast<double>(k) / total);
            return !cancel.load();
        };

        auto solver = makeSolver(kind, s.epsilon);
        solver->denoise(res.noisy, run.result, s.lambda, s.iterations, nullptr, &observer);
        run.totalMs = std::chrono::duration<double, std::milli>(clock::now() - t0).count();
        return run;
    }
};
