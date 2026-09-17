#pragma once
#include <gui/StandardTabView.h>
#include "DenoisingView.h"
#include "ConvergenceView.h"
#include "LambdaStudyView.h"
#include "IterationLogView.h"
#include "LogView.h"

// ============================================================
// ResultTabs — the work area, one page per question:
//   Overview    : how good is each method?        (cards + images)
//   Convergence : how does each method get there?  (per-iteration plots)
//   Lambda study: which lambda should be used?      (PSNR, L-curve, cost)
//   Report      : exact numbers                     (tables)
//   Log         : what was run in this session
// Views receive immutable results only; they never run solvers.
// ============================================================
class ResultTabs : public gui::StandardTabView
{
    DenoisingView    _overview;
    ConvergenceView  _convergence;
    LambdaStudyView  _study;
    IterationLogView _report;
    LogView          _log;

public:
    enum Page { Overview = 0, Convergence, Study, Report, Log };

    ResultTabs()
    {
        addView(&_overview, "Overview");
        addView(&_convergence, "Convergence");
        addView(&_study, "Lambda study");
        addView(&_report, "Report");
        addView(&_log, "Log");
    }

    void showInput(const ImageData& original, const ImageData& noisy, double noisyPsnr, const DenoiseSettings& s)
    {
        _overview.showInput(original, noisy, noisyPsnr, s);
    }

    void setResult(const DenoiseResultPtr& r)
    {
        if (!r) return;
        if (r->mode == RunMode::LambdaStudy)
        {
            _study.setResult(r);
            _report.setStudy(*r);
            return;
        }
        _overview.showResult(*r);
        _convergence.setResult(r);
        _report.setComparison(*r);
    }

    LogView& log() { return _log; }

    void setCurrentLambda(double lambda) { _study.setCurrentLambda(lambda); }

    gui::Canvas* currentCanvas()
    {
        switch (getCurrentViewPos())
        {
            case Overview:    return &_overview;
            case Convergence: return &_convergence;
            case Study:       return &_study;
            default:          return nullptr;
        }
    }

    void showPage(Page p) { setCurrentViewPos((int) p); }
};
