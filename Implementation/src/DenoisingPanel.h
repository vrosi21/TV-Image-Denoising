#pragma once
#include <gui/View.h>
#include <gui/SplitterLayout.h>
#include <gui/Thread.h>
#include <gui/Types.h>          // gui::getResFileName
#include "ControlsPanel.h"
#include "ResultTabs.h"
#include "DenoisingRunner.h"
#include "AppFolders.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <memory>
#include <thread>

// ============================================================
// DenoisingPanel — central view and experiment coordinator
//
// Widget tree:
//   SplitterLayout (Horizontal, AuxiliaryCell::First)
//     ├── ControlsPanel (settings sidebar, left)
//     └── ResultTabs    (Overview | Convergence | Lambda study | Report | Log)
//
// Runs execute on a worker thread (DenoisingRunner); progress and
// the finished, immutable result come back to the main thread via
// gui::thread::asyncExecInMainThread, so the window stays
// responsive and Stop works at any time.
//
// SRP: coordinates image loading, noise preview, run scheduling
//      and result distribution.  Maths → DenoisingRunner/solvers,
//      rendering → views, settings → ControlsPanel.
// ============================================================
class DenoisingPanel : public gui::View
{
private:
    gui::SplitterLayout _sl;
    ControlsPanel       _controls;
    ResultTabs          _tabs;

    ImageData   _original;
    std::string _imageName;
    bool        _hasImage = false;

    std::thread          _worker;
    std::atomic<bool>    _cancel { false };
    bool                 _running = false;
    bool                 _rerunPending = false;
    std::shared_ptr<int> _alive = std::make_shared<int>(1);
    DenoiseResultPtr     _lastComparison;

    std::function<void(const td::String&)> _onStatus;
    std::function<void(double)>            _onProgress;   // < 0 = idle
    std::function<void(const td::String&)> _onImageName;

    // Resource keys matching <FileNames> entries in res/main.xml
    static constexpr const char* kImageKeys[3]  = { "cameraman", "ape", "barbara" };
    static constexpr const char* kImageNames[3] = { "Cameraman", "Ape", "Barbara" };

    void status(const td::String& s) { if (_onStatus) _onStatus(s); }

    // ----------------------------------------------------------
    // Input: image + noisy preview (same seed/sigma as the next run)
    // ----------------------------------------------------------
    void previewInput()
    {
        if (!_hasImage) return;
        const DenoiseSettings s = _controls.settings();
        ImageData noisy = DenoisingRunner::makeNoisy(_original, s.sigma, s.seed);
        _tabs.showInput(_original, noisy, metrics::psnr(noisy, _original), s);
    }

    void onImageLoaded(const std::string& name)
    {
        _hasImage = true;
        _imageName = name;
        _lastComparison.reset();
        _lastStudy.reset();
        _exportDir.clear();
        const td::String info = plot::fmt("%d %s %d px, grayscale", _original.width, plot::glyph::times, _original.height);
        _controls.setImageInfo(info);
        if (_onImageName) _onImageName(plot::fmt("%s  %s  %s", name.c_str(), plot::glyph::dot, info.c_str()));
        _tabs.log().entry("Loaded image %s (%d x %d)", name.c_str(), _original.width, _original.height);
        previewInput();
        _tabs.showPage(ResultTabs::Overview);
        start(RunMode::Comparison);
    }

    bool loadFromPath(const std::string& path, const std::string& name)
    {
        if (!_original.loadFromFile(path))
        {
            showAlert(td::String("Could not open image"), plot::fmt("The file could not be read:\n%s", path.c_str()));
            return false;
        }
        onImageLoaded(name);
        return true;
    }

    // ----------------------------------------------------------
    // Run scheduling
    // ----------------------------------------------------------
    void start(RunMode mode)
    {
        if (!_hasImage) return;
        if (_running)
        {
            if (mode == RunMode::Comparison) _rerunPending = true;
            return;
        }

        const DenoiseSettings settings = _controls.settings();
        _cancel = false;
        _running = true;
        if (_onProgress) _onProgress(0.0);
        status(td::String(mode == RunMode::LambdaStudy ? "Running lambda study..." : "Denoising..."));
        if (mode == RunMode::LambdaStudy) _tabs.showPage(ResultTabs::Study);

        const ImageData original = _original;
        const std::string name = _imageName;
        std::weak_ptr<int> alive = _alive;

        _worker = std::thread([this, original, name, settings, mode, alive]()
        {
            auto lastPost = std::chrono::steady_clock::now() - std::chrono::seconds(1);
            auto progress = [&](const std::string& stage, double fraction)
            {
                const auto now = std::chrono::steady_clock::now();
                if (now - lastPost < std::chrono::milliseconds(60)) return;
                lastPost = now;
                td::String text = plot::fmt("%s  %.0f%%", stage.c_str(), 100.0 * fraction);
                gui::thread::asyncExecInMainThread([this, alive, text, fraction]()
                {
                    if (alive.expired() || !_running) return;
                    if (_onProgress) _onProgress(fraction);
                    status(text);
                });
            };

            DenoiseResultPtr result;
            td::String error;
            try
            {
                result = std::make_shared<const DenoiseResult>(
                    DenoisingRunner::run(original, name, settings, mode, _cancel, progress));
            }
            catch (const std::exception& ex) { error = td::String(ex.what()); }
            catch (...)                      { error = td::String("unknown error"); }

            gui::thread::asyncExecInMainThread([this, alive, result, error]()
            {
                if (alive.expired()) return;
                onFinished(result, error);
            });
        });
    }

    void onFinished(const DenoiseResultPtr& res, const td::String& error)
    {
        if (_worker.joinable()) _worker.join();
        _running = false;
        if (_onProgress) _onProgress(-1.0);

        if (error.length() > 0)
        {
            status(plot::fmt("Run failed: %s", error.c_str()));
            _tabs.log().entry("Run failed: %s", error.c_str());
            showAlert(td::String("Denoising error"), error);
        }
        else if (res)
        {
            if (res->mode == RunMode::Comparison) _lastComparison = res;
            else _lastStudy = res;
            if (!_exporting) _exportDir.clear();     // new result -> new export folder
            _tabs.setResult(res);
            logResult(*res);
            status(summary(*res));
        }

        if (_rerunPending)
        {
            _rerunPending = false;
            start(RunMode::Comparison);
        }
    }

    td::String summary(const DenoiseResult& r) const
    {
        const char* prefix = r.cancelled ? "Stopped.  " : "";
        if (r.mode == RunMode::LambdaStudy)
        {
            td::String s = plot::fmt("%sLambda study done", prefix);
            for (auto& series : r.study)
            {
                const int b = series.bestIndex();
                if (b >= 0)
                    s += plot::fmt("   %s   %s best %s = %.3g (%.2f dB)", plot::glyph::dot, solverShortName(series.kind),
                                   plot::glyph::lambda, series.points[(size_t) b].lambda, series.points[(size_t) b].psnr);
            }
            return s;
        }
        td::String s = plot::fmt("%sNoisy %.2f dB", prefix, r.noisyPsnr);
        for (auto& run : r.runs)
            s += plot::fmt("   %s   %s %.2f dB in %.0f ms", plot::glyph::dot, solverShortName(run.kind), run.last().psnr, run.totalMs);
        return s;
    }

    void logResult(const DenoiseResult& r)
    {
        LogView& log = _tabs.log();
        const DenoiseSettings& s = r.settings;
        log.entry("%s on %s%s", r.mode == RunMode::LambdaStudy ? "Lambda study" : "Comparison",
                  r.imageName.c_str(), r.cancelled ? "  [stopped]" : "");
        log.detail("sigma %.3f, seed %u, lambda %.3f, epsilon %.3f, %d iterations  ->  noisy PSNR %.2f dB",
                   s.sigma, s.seed, s.lambda, s.epsilon, s.iterations, r.noisyPsnr);

        if (r.mode == RunMode::LambdaStudy)
        {
            log.detail("lambda from %.3g to %.3g, %d points", s.studyFrom, s.studyTo, s.studyPoints);
            for (auto& series : r.study)
            {
                const int b = series.bestIndex();
                if (b >= 0)
                    log.detail("%-18s best lambda %.4f  ->  %.2f dB", solverName(series.kind),
                               series.points[(size_t) b].lambda, series.points[(size_t) b].psnr);
            }
            return;
        }
        for (auto& run : r.runs)
            log.detail("%-18s %.2f dB (%+.2f dB), energy %.3f, %d iterations, %.1f ms", solverName(run.kind),
                       run.last().psnr, run.last().psnr - r.noisyPsnr, run.last().energy, run.last().k, run.totalMs);
    }

    // ----------------------------------------------------------
    // Files (no dialogs — see AppFolders.h)
    // ----------------------------------------------------------
    std::vector<fo::fs::path> _userImages;
    DenoiseResultPtr          _lastStudy;
    fo::fs::path              _exportDir;        // folder of the current result, created on first export
    std::vector<int>          _exportQueue;      // pages still to export in "Export all"
    int                       _exportFiles = 0;
    int                       _exportReturnPage = 0;
    bool                      _exporting = false;

    static const char* pageStem(int page)
    {
        switch (page)
        {
            case ResultTabs::Overview:    return "overview";
            case ResultTabs::Convergence: return "convergence";
            case ResultTabs::Study:       return "lambda-study";
            default:                      return "chart";
        }
    }

    // One folder per result: every export of the same run lands together.
    const fo::fs::path& exportDir()
    {
        std::error_code ec;
        if (_exportDir.empty() || !fo::fs::is_directory(_exportDir, ec))
            _exportDir = appfs::createExportFolder(_imageName.empty() ? std::string("export") : _imageName);
        return _exportDir;
    }

    // Runs fn on the main thread after a short delay (lets a page become visible).
    void later(int ms, std::function<void()> fn)
    {
        std::weak_ptr<int> alive = _alive;
        std::thread([alive, ms, fn]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            gui::thread::asyncExecInMainThread([alive, fn]() { if (!alive.expired()) fn(); });
        }).detach();
    }

    static bool exportCanvas(gui::Canvas* canvas, const fo::fs::path& target, bool pdf)
    {
        if (!canvas) return false;
        fo::fs::path base = target;
        base.replace_extension("");
        const td::String baseName(appfs::toUtf8(base).c_str());
        const bool ok = pdf ? canvas->exportToPDF(baseName, true) : canvas->exportToSVG(baseName, true);

        // natID appends the extension itself; keep a fallback if it does not
        std::error_code ec;
        if (ok && !fo::fs::exists(target, ec) && fo::fs::exists(base, ec))
            fo::fs::rename(base, target, ec);
        return ok && fo::fs::exists(target, ec);
    }

    int writeImages(const fo::fs::path& dir)
    {
        if (!_lastComparison) return 0;
        const DenoiseResult& r = *_lastComparison;
        int saved = 0;
        auto save = [&](const ImageData& img, const char* name)
        {
            if (img.saveToPng(appfs::toUtf8(appfs::uniqueFile(dir, name, ".png")))) ++saved;
        };
        save(r.original, "original");
        save(r.noisy, "noisy");
        for (auto& run : r.runs) save(run.result, run.kind == SolverKind::GD ? "denoised_gd" : "denoised_newton");
        return saved;
    }

    int writeData(const fo::fs::path& dir)
    {
        int files = 0;
        if (_lastComparison)
        {
            const DenoiseResult& r = *_lastComparison;
            std::ofstream f(appfs::uniqueFile(dir, "iterations", ".csv"));
            f << "# image " << r.imageName << ", sigma " << r.settings.sigma << ", seed " << r.settings.seed
              << ", lambda " << r.settings.lambda << ", epsilon " << r.settings.epsilon << "\n";
            f << "method,iteration,energy,psnr_db,relative_change,time_ms\n";
            for (auto& run : r.runs)
                for (auto& rec : run.iterations)
                    f << solverShortName(run.kind) << ',' << rec.k << ',' << rec.energy << ',' << rec.psnr << ','
                      << (rec.k == 0 ? 0.0 : rec.relChange) << ',' << rec.timeMs << '\n';
            if (f.good()) ++files;
        }
        if (_lastStudy)
        {
            const DenoiseResult& r = *_lastStudy;
            std::ofstream f(appfs::uniqueFile(dir, "lambda-study", ".csv"));
            f << "# image " << r.imageName << ", sigma " << r.settings.sigma << ", seed " << r.settings.seed
              << ", iterations " << r.settings.iterations << "\n";
            f << "method,lambda,psnr_db,data_misfit,total_variation,time_ms\n";
            for (auto& series : r.study)
                for (auto& p : series.points)
                    f << solverShortName(series.kind) << ',' << p.lambda << ',' << p.psnr << ',' << p.fidelity << ','
                      << p.tv << ',' << p.timeMs << '\n';
            if (f.good()) ++files;
        }
        return files;
    }

    void reportExport(const char* what, int files)
    {
        const std::string dir = appfs::toUtf8(_exportDir);
        status(plot::fmt("%s: %d file(s) in %s", what, files, dir.c_str()));
        _tabs.log().entry("%s: %d file(s) written to %s", what, files, dir.c_str());
    }

    // "Export all": show each chart page, wait until natID can render it, export it.
    void exportNextPage(int attempt)
    {
        if (_exportQueue.empty())
        {
            _tabs.showPage((ResultTabs::Page) _exportReturnPage);
            _exporting = false;
            reportExport("Exported everything", _exportFiles);
            return;
        }
        const int page = _exportQueue.front();
        if (_tabs.getCurrentViewPos() != page) _tabs.showPage((ResultTabs::Page) page);

        later(150, [this, page, attempt]()
        {
            const fo::fs::path target = appfs::uniqueFile(exportDir(), pageStem(page), ".pdf");
            if (exportCanvas(_tabs.currentCanvas(), target, true))
                ++_exportFiles;
            else if (attempt < 20)
            {
                exportNextPage(attempt + 1);    // page not ready yet: try again shortly
                return;
            }
            else
                _tabs.log().entry("Could not export the %s page", pageStem(page));

            _exportQueue.erase(_exportQueue.begin());
            exportNextPage(0);
        });
    }

public:
    DenoisingPanel()
    : _sl(gui::SplitterLayout::Orientation::Horizontal,
          gui::SplitterLayout::AuxiliaryCell::First)
    {
        _controls.setOnSelectImage([this](int idx) { selectImage(idx); });
        _controls.setOnRefreshImages([this]()      { refreshImages(true); });
        _controls.setOnInputChanged([this]()       { if (!_controls.autoRun()) previewInput(); });
        _controls.setOnLambdaChanged([this](double l) { _tabs.setCurrentLambda(l); });
        _controls.setOnSettingsChanged([this]()
        {
            if (_controls.autoRun() && _hasImage) start(RunMode::Comparison);
        });

        setMargins(0, 0, 0, 0);
        _sl.setMargins(0, 0);
        _sl.setContent(_controls, _tabs);
        setLayout(&_sl);
    }

    ~DenoisingPanel()
    {
        _alive.reset();
        _cancel = true;
        if (_worker.joinable()) _worker.join();
    }

    void onInitialAppearance() override
    {
        refreshImages(false);
        selectImage(0);
    }

    // ---- wiring to the window chrome -------------------------
    void setOnStatus(const std::function<void(const td::String&)>& f)    { _onStatus = f; }
    void setOnProgress(const std::function<void(double)>& f)             { _onProgress = f; }
    void setOnImageName(const std::function<void(const td::String&)>& f) { _onImageName = f; }

    // ---- images ------------------------------------------------
    // Built-in samples first, then PNG/JPEG files from the Images folder.
    void selectImage(int idx)
    {
        if (idx >= 0 && idx < 3)
        {
            td::String path = gui::getResFileName(kImageKeys[idx]);
            loadFromPath(path.c_str(), kImageNames[idx]);
        }
        else if (idx >= 3 && idx - 3 < (int) _userImages.size())
        {
            const fo::fs::path& p = _userImages[(size_t) (idx - 3)];
            loadFromPath(appfs::toUtf8(p), appfs::toUtf8(p.filename()));
        }
    }

    void refreshImages(bool report)
    {
        _userImages = appfs::listImages();
        std::vector<td::String> names;
        for (auto& p : _userImages) names.push_back(td::String(appfs::toUtf8(p.filename()).c_str()));
        _controls.setUserImages(names);

        const std::string folder = appfs::toUtf8(appfs::imagesFolder());
        _controls.setImagesFolderHint(td::String(folder.c_str()));
        if (report)
        {
            status(plot::fmt("%d image(s) found in %s", (int) _userImages.size(), folder.c_str()));
            _tabs.log().entry("Images folder %s: %d image(s)", folder.c_str(), (int) _userImages.size());
        }
    }

    void openImagesFolder()
    {
        if (!appfs::openFolder(appfs::imagesFolder()))
            showAlert(td::String("Images folder"), td::String(appfs::toUtf8(appfs::imagesFolder()).c_str()));
    }

    // ---- runs --------------------------------------------------
    void runComparison()  { start(RunMode::Comparison); _tabs.showPage(ResultTabs::Overview); }
    void runLambdaStudy() { start(RunMode::LambdaStudy); }

    void stop()
    {
        if (!_running) return;
        _rerunPending = false;
        _cancel = true;
        status(td::String("Stopping..."));
    }

    // Blocks until the worker has stopped (used before closing).
    void cancelAndWait()
    {
        _rerunPending = false;
        _cancel = true;
        if (_worker.joinable()) _worker.join();
        _running = false;
    }

    // ---- exports ------------------------------------------------
    void exportAll()
    {
        if (_exporting) return;
        if (!_lastComparison && !_lastStudy)
        {
            showAlert(td::String("Export"), td::String("Run a comparison or a lambda study first."));
            return;
        }
        _exportDir.clear();                      // every "Export all" gets its own dated folder
        exportDir();
        _exporting = true;
        _exportReturnPage = _tabs.getCurrentViewPos();
        _exportFiles = writeImages(_exportDir) + writeData(_exportDir);
        _exportQueue.clear();
        if (_lastComparison) { _exportQueue.push_back(ResultTabs::Overview); _exportQueue.push_back(ResultTabs::Convergence); }
        if (_lastStudy) _exportQueue.push_back(ResultTabs::Study);
        status(td::String("Exporting charts..."));
        exportNextPage(0);
    }

    void exportCurrentChart(bool pdf)
    {
        const int page = _tabs.getCurrentViewPos();
        if (!_tabs.currentCanvas())
        {
            showAlert(td::String("Export chart"), td::String("Open the Overview, Convergence or Lambda study page first."));
            return;
        }
        const fo::fs::path target = appfs::uniqueFile(exportDir(), pageStem(page), pdf ? ".pdf" : ".svg");
        if (exportCanvas(_tabs.currentCanvas(), target, pdf))
            reportExport(pdf ? "Chart exported (PDF)" : "Chart exported (SVG)", 1);
        else
            showAlert(td::String("Export chart"), td::String("The chart could not be exported."));
    }

    void saveDenoisedImages()
    {
        if (!_lastComparison || _lastComparison->runs.empty())
        {
            showAlert(td::String("Save images"), td::String("Run a comparison first."));
            return;
        }
        reportExport("Images saved", writeImages(exportDir()));
    }

    void exportData()
    {
        if (!_lastComparison && !_lastStudy)
        {
            showAlert(td::String("Export data"), td::String("Run a comparison or a lambda study first."));
            return;
        }
        reportExport("Data exported (CSV)", writeData(exportDir()));
    }

    void openExportsFolder()
    {
        std::error_code ec;
        const fo::fs::path dir = (!_exportDir.empty() && fo::fs::is_directory(_exportDir, ec)) ? _exportDir : appfs::exportsFolder();
        if (!appfs::openFolder(dir))
            showAlert(td::String("Exports folder"), td::String(appfs::toUtf8(dir).c_str()));
    }
};
