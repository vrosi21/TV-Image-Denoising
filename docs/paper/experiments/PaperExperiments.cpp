// TVPaperExperiments: runs the experiments reported in the paper and writes
// CSV tables and PNG images.  Solvers, noise generation and metrics are the
// application's own code (DenoisingRunner, TVSolverGD, TVSolverNewton).
//
// usage:  TVPaperExperiments <Implementation dir> <output dir> [experiment ...]
//         experiments: comparison lambda epsilon sigma scaling   (default: all)
//
// Solver progress is printed to stdout; experiment progress goes to stderr.

#include <mu/Application.h>
#include "DenoisingRunner.h"
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{

const char* kImages[] = { "cameraman", "barbara", "ape" };

constexpr float    kLambda = 0.10f;
constexpr float    kSigma  = 0.10f;
constexpr float    kEps    = 0.01f;
constexpr unsigned kSeed   = 12345;

std::atomic<bool> g_noCancel{ false };

DenoiseSettings baseSettings(bool gd, bool newton, int iterations)
{
    DenoiseSettings s;
    s.runGD = gd;
    s.runNewton = newton;
    s.lambda = kLambda;
    s.sigma = kSigma;
    s.epsilon = kEps;
    s.seed = kSeed;
    s.iterations = iterations;
    return s;
}

DenoiseResult run(const ImageData& img, const std::string& name, const DenoiseSettings& s, RunMode mode)
{
    return DenoisingRunner::run(img, name, s, mode, g_noCancel, nullptr);
}

ImageData loadImage(const fs::path& implDir, const char* name)
{
    ImageData img;
    const fs::path p = implDir / "res" / "images" / (std::string(name) + ".png");
    if (!img.loadFromFile(p.string()))
    {
        std::fprintf(stderr, "cannot load %s\n", p.string().c_str());
        std::exit(2);
    }
    return img;
}

// bilinear resampling to n x n (used only for the timing experiment)
ImageData resample(const ImageData& src, int n)
{
    ImageData out(n, n);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
        {
            const double y = (i + 0.5) * src.height / n - 0.5, x = (j + 0.5) * src.width / n - 0.5;
            const int y0 = (std::max)(0, (std::min)(src.height - 1, (int) std::floor(y)));
            const int x0 = (std::max)(0, (std::min)(src.width - 1, (int) std::floor(x)));
            const int y1 = (std::min)(src.height - 1, y0 + 1), x1 = (std::min)(src.width - 1, x0 + 1);
            const double ty = (std::min)(1.0, (std::max)(0.0, y - y0)), tx = (std::min)(1.0, (std::max)(0.0, x - x0));
            const double top = (1 - tx) * src.at(y0, x0) + tx * src.at(y0, x1);
            const double bot = (1 - tx) * src.at(y1, x0) + tx * src.at(y1, x1);
            out.at(i, j) = (float) ((1 - ty) * top + ty * bot);
        }
    return out;
}

std::ofstream openCsv(const fs::path& dir, const char* file, const char* header)
{
    std::ofstream o(dir / file);
    o.precision(10);
    o << header << '\n';
    return o;
}

void progress(const char* fmt, const std::string& a, double b = 0)
{
    std::fprintf(stderr, fmt, a.c_str(), b);
    std::fflush(stderr);
}

// ------------------------------------------------------------------
// 1. Both methods on the three test images at the default parameters
// ------------------------------------------------------------------
void comparison(const fs::path& implDir, const fs::path& out)
{
    auto conv = openCsv(out, "convergence.csv", "image,method,k,energy,psnr,rel_change,time_ms");
    auto summ = openCsv(out, "comparison_summary.csv", "image,noisy_psnr,method,iterations,final_psnr,final_energy,total_ms");
    const fs::path imgDir = out / "images";
    fs::create_directories(imgDir);

    for (const char* name : kImages)
    {
        progress("comparison: %s\n", name);
        const ImageData original = loadImage(implDir, name);

        const DenoiseResult gd = run(original, name, baseSettings(true, false, 300), RunMode::Comparison);
        const DenoiseResult nt = run(original, name, baseSettings(false, true, 30), RunMode::Comparison);

        original.saveToPng((imgDir / (std::string(name) + "_original.png")).string());
        gd.noisy.saveToPng((imgDir / (std::string(name) + "_noisy.png")).string());

        for (const DenoiseResult* r : { &gd, &nt })
        {
            const SolverRun& sr = r->runs.front();
            const std::string method = solverShortName(sr.kind);
            for (const IterationRecord& it : sr.iterations)
                conv << name << ',' << method << ',' << it.k << ',' << it.energy << ',' << it.psnr << ','
                     << it.relChange << ',' << it.timeMs << '\n';
            summ << name << ',' << r->noisyPsnr << ',' << method << ',' << sr.last().k << ',' << sr.last().psnr << ','
                 << sr.last().energy << ',' << sr.totalMs << '\n';
            sr.result.saveToPng((imgDir / (std::string(name) + "_" + (sr.kind == SolverKind::GD ? "gd" : "newton") + ".png")).string());
        }

        // equal-iteration snapshot: both methods after 10 iterations (application default)
        const DenoiseResult both10 = run(original, name, baseSettings(true, true, 10), RunMode::Comparison);
        for (const SolverRun& sr : both10.runs)
            sr.result.saveToPng((imgDir / (std::string(name) + "_" + (sr.kind == SolverKind::GD ? "gd" : "newton") + "_k10.png")).string());
    }
}

// ------------------------------------------------------------------
// 2. Lambda study at sigma = 0.1 (application Lambda study, more points)
// ------------------------------------------------------------------
void lambdaStudy(const fs::path& implDir, const fs::path& out)
{
    auto csv = openCsv(out, "lambda_study.csv", "image,method,iterations,lambda,psnr,fidelity,tv,time_ms");
    for (const char* name : kImages)
    {
        const ImageData original = loadImage(implDir, name);
        for (SolverKind kind : { SolverKind::GD, SolverKind::Newton })
        {
            progress("lambda study: %s\n", std::string(name) + " " + solverShortName(kind));
            const int iterations = kind == SolverKind::GD ? 200 : 20;
            DenoiseSettings s = baseSettings(kind == SolverKind::GD, kind == SolverKind::Newton, iterations);
            s.studyFrom = 0.01;
            s.studyTo = 1.0;
            s.studyPoints = 15;
            const DenoiseResult r = run(original, name, s, RunMode::LambdaStudy);
            for (const StudyPoint& p : r.study.front().points)
                csv << name << ',' << solverShortName(kind) << ',' << iterations << ',' << p.lambda << ',' << p.psnr << ','
                    << p.fidelity << ',' << p.tv << ',' << p.timeMs << '\n';
        }
    }
}

// ------------------------------------------------------------------
// 3. Smoothing parameter of the Newton-type method
// ------------------------------------------------------------------
void epsilonStudy(const fs::path& implDir, const fs::path& out)
{
    auto csv = openCsv(out, "epsilon_study.csv", "image,epsilon,k,energy,psnr,rel_change,time_ms");
    const float eps[] = { 0.001f, 0.002f, 0.005f, 0.01f, 0.02f, 0.05f, 0.1f };
    for (const char* name : kImages)
    {
        const ImageData original = loadImage(implDir, name);
        for (float e : eps)
        {
            progress("epsilon study: %s  eps %g\n", name, e);
            DenoiseSettings s = baseSettings(false, true, 25);
            s.epsilon = e;
            const DenoiseResult r = run(original, name, s, RunMode::Comparison);
            for (const IterationRecord& it : r.runs.front().iterations)
                csv << name << ',' << e << ',' << it.k << ',' << it.energy << ',' << it.psnr << ',' << it.relChange << ','
                    << it.timeMs << '\n';
        }
    }
}

// ------------------------------------------------------------------
// 4. Best lambda for several noise levels
// ------------------------------------------------------------------
void sigmaStudy(const fs::path& implDir, const fs::path& out)
{
    auto csv = openCsv(out, "sigma_study.csv", "image,sigma,noisy_psnr,method,iterations,lambda,psnr,time_ms");
    const float sigmas[] = { 0.05f, 0.10f, 0.15f, 0.20f };
    for (const char* name : kImages)
    {
        const ImageData original = loadImage(implDir, name);
        for (float sigma : sigmas)
            for (SolverKind kind : { SolverKind::GD, SolverKind::Newton })
            {
                progress("sigma study: %s  sigma %g\n", std::string(name) + " " + solverShortName(kind), sigma);
                const int iterations = kind == SolverKind::GD ? 200 : 15;
                DenoiseSettings s = baseSettings(kind == SolverKind::GD, kind == SolverKind::Newton, iterations);
                s.sigma = sigma;
                s.studyFrom = 0.01;
                s.studyTo = 1.0;
                s.studyPoints = 11;
                const DenoiseResult r = run(original, name, s, RunMode::LambdaStudy);
                for (const StudyPoint& p : r.study.front().points)
                    csv << name << ',' << sigma << ',' << r.noisyPsnr << ',' << solverShortName(kind) << ',' << iterations << ','
                        << p.lambda << ',' << p.psnr << ',' << p.timeMs << '\n';
            }
    }
}

// ------------------------------------------------------------------
// 5. Cost of one iteration as a function of the number of pixels
// ------------------------------------------------------------------
void scaling(const fs::path& implDir, const fs::path& out)
{
    auto csv = openCsv(out, "scaling.csv", "n,N,method,linear_solver,iterations,ms_per_iteration");
    const ImageData cameraman = loadImage(implDir, "cameraman");
    const int sizes[] = { 8, 12, 16, 24, 32, 40, 48, 56, 64, 72, 96, 128, 160, 225, 320, 450 };

    for (int n : sizes)
    {
        const ImageData img = n == cameraman.width ? cameraman : resample(cameraman, n);
        const int N = n * n;
        for (SolverKind kind : { SolverKind::GD, SolverKind::Newton })
        {
            progress("scaling: %s  n %g\n", solverShortName(kind), n);
            const int iterations = kind == SolverKind::GD ? 20 : (N >= 2304 && N <= TVSolverNewton::kMaxDenseN ? 2 : 4);
            const DenoiseResult r = run(img, "scaled", baseSettings(kind == SolverKind::GD, kind == SolverKind::Newton, iterations), RunMode::Comparison);
            const SolverRun& sr = r.runs.front();

            // per-iteration wall time without the first iteration (warm-up)
            double ms = 0;
            int count = 0;
            for (size_t i = 2; i < sr.iterations.size(); ++i, ++count)
                ms += sr.iterations[i].timeMs - sr.iterations[i - 1].timeMs;
            if (count == 0) { ms = sr.iterations.back().timeMs; count = 1; }

            const char* solver = kind == SolverKind::GD ? "none" : (N <= TVSolverNewton::kMaxDenseN ? "dense LU" : "sparse LDLT");
            csv << n << ',' << N << ',' << solverShortName(kind) << ',' << solver << ',' << iterations << ',' << ms / count << '\n';
            csv.flush();
        }
    }
}

} // namespace

int main(int argc, const char* argv[])
{
    mu::Application app(argc, argv);

    if (argc < 3)
    {
        std::fprintf(stderr, "usage: %s <Implementation dir> <output dir> [comparison lambda epsilon sigma scaling]\n", argv[0]);
        return 1;
    }
    const fs::path implDir = argv[1];
    const fs::path out = argv[2];
    fs::create_directories(out);

    std::set<std::string> selected;
    for (int i = 3; i < argc; ++i) selected.insert(argv[i]);
    auto wanted = [&](const char* e) { return selected.empty() || selected.count(e) > 0; };

    if (wanted("comparison")) comparison(implDir, out);
    if (wanted("lambda"))     lambdaStudy(implDir, out);
    if (wanted("epsilon"))    epsilonStudy(implDir, out);
    if (wanted("sigma"))      sigmaStudy(implDir, out);
    if (wanted("scaling"))    scaling(implDir, out);

    std::fprintf(stderr, "done\n");
    return 0;
}
