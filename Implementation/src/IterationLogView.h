#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/TableEdit.h>
#include <gui/VerticalLayout.h>
#include <dp/IDataSet.h>
#include <dp/IDatabase.h>
#include "PlotStyle.h"
#include <limits>

// ============================================================
// IterationLogView — the Report page, backed by in-memory
// (connectionless) data sets:
//   comparison : one row per method of the last comparison
//   iterations : one row per iteration of every method
//   lambda study: one row per (method, lambda) of the last study
// ============================================================
class IterationLogView : public gui::View
{
    gui::Label          _lblSummary;
    gui::TableEdit      _summary;
    gui::Label          _lblDetail;
    gui::TableEdit      _detail;
    gui::Label          _lblStudy;
    gui::TableEdit      _study;
    gui::VerticalLayout _vl;
    dp::IDataSetPtr     _dsSummary, _dsDetail, _dsStudy;

    enum { S_Method = 0, S_Psnr, S_Gain, S_Energy, S_Iters, S_Ms, S_Count };
    enum { D_Method = 0, D_K, D_Energy, D_Psnr, D_Rel, D_Ms, D_Count };
    enum { W_Method = 0, W_Lambda, W_Psnr, W_Fid, W_TV, W_Ms, W_Count };

    void initSummary()
    {
        _dsSummary = dp::createConnectionlessDataSet(dp::IDataSet::Size::Small);
        dp::DSColumns cols(_dsSummary->allocBindColumns(S_Count));
        cols << "Method" << td::string8 << "Psnr" << td::real8 << "Gain" << td::real8
             << "Energy" << td::real8 << "Iters" << td::int4 << "Ms" << td::real8;
        _dsSummary->execute();

        gui::Columns vis(_summary.allocBindColumns(S_Count));
        vis << gui::ThSep::DoNotShowThSep
            << gui::Header(S_Method, "Method", "Denoising algorithm", 170)
            << gui::Header(S_Psnr, "PSNR [dB]", "Peak signal-to-noise ratio against the clean image", 100, td::HAlignment::Right)
            << gui::Header(S_Gain, "Gain [dB]", "PSNR improvement over the noisy input", 100, td::HAlignment::Right)
            << gui::Header(S_Energy, "Final energy", "F(u) = 1/2 |u - f|^2 + lambda TV(u), exact TV", 120, td::HAlignment::Right)
            << gui::Header(S_Iters, "Iterations", "Iterations performed", 90, td::HAlignment::Right)
            << gui::Header(S_Ms, "Time [ms]", "Wall-clock solver time", 100, td::HAlignment::Right);
        _summary.init(_dsSummary);
        _summary.setColumnNumericFormat(S_Psnr, td::FormatFloat::Decimal, 2);
        _summary.setColumnNumericFormat(S_Gain, td::FormatFloat::Decimal, 2);
        _summary.setColumnNumericFormat(S_Energy, td::FormatFloat::Decimal, 3);
        _summary.setColumnNumericFormat(S_Ms, td::FormatFloat::Decimal, 1);
    }

    void initDetail()
    {
        _dsDetail = dp::createConnectionlessDataSet(dp::IDataSet::Size::Medium);
        dp::DSColumns cols(_dsDetail->allocBindColumns(D_Count));
        cols << "Method" << td::string8 << "K" << td::int4 << "Energy" << td::real8
             << "Psnr" << td::real8 << "Rel" << td::real8 << "Ms" << td::real8;
        _dsDetail->execute();

        gui::Columns vis(_detail.allocBindColumns(D_Count));
        vis << gui::ThSep::DoNotShowThSep
            << gui::Header(D_Method, "Method", "Denoising algorithm", 170)
            << gui::Header(D_K, "k", "Iteration (0 = noisy input)", 50, td::HAlignment::Right)
            << gui::Header(D_Energy, "Energy F(u_k)", "Exact TV energy of the iterate", 120, td::HAlignment::Right)
            << gui::Header(D_Psnr, "PSNR [dB]", "Quality of the iterate", 100, td::HAlignment::Right)
            << gui::Header(D_Rel, "Relative change", "|u_k - u_k-1| / |u_k-1|", 120, td::HAlignment::Right)
            << gui::Header(D_Ms, "Time [ms]", "Wall time since the solver started", 100, td::HAlignment::Right);
        _detail.init(_dsDetail);
        _detail.setColumnNumericFormat(D_Energy, td::FormatFloat::Decimal, 3);
        _detail.setColumnNumericFormat(D_Psnr, td::FormatFloat::Decimal, 2);
        _detail.setColumnNumericFormat(D_Rel, td::FormatFloat::Scientific, 2);
        _detail.setColumnNumericFormat(D_Ms, td::FormatFloat::Decimal, 1);
    }

    void initStudy()
    {
        _dsStudy = dp::createConnectionlessDataSet(dp::IDataSet::Size::Medium);
        dp::DSColumns cols(_dsStudy->allocBindColumns(W_Count));
        cols << "Method" << td::string8 << "Lambda" << td::real8 << "Psnr" << td::real8
             << "Fid" << td::real8 << "TV" << td::real8 << "Ms" << td::real8;
        _dsStudy->execute();

        gui::Columns vis(_study.allocBindColumns(W_Count));
        vis << gui::ThSep::DoNotShowThSep
            << gui::Header(W_Method, "Method", "Denoising algorithm", 170)
            << gui::Header(W_Lambda, "lambda", "Regularisation weight", 90, td::HAlignment::Right)
            << gui::Header(W_Psnr, "PSNR [dB]", "Quality of the denoised image", 100, td::HAlignment::Right)
            << gui::Header(W_Fid, "Data misfit", "1/2 |u - f|^2", 110, td::HAlignment::Right)
            << gui::Header(W_TV, "TV(u)", "Total variation of the result", 110, td::HAlignment::Right)
            << gui::Header(W_Ms, "Time [ms]", "Solver wall time", 100, td::HAlignment::Right);
        _study.init(_dsStudy);
        _study.setColumnNumericFormat(W_Lambda, td::FormatFloat::Decimal, 4);
        _study.setColumnNumericFormat(W_Psnr, td::FormatFloat::Decimal, 2);
        _study.setColumnNumericFormat(W_Fid, td::FormatFloat::Decimal, 3);
        _study.setColumnNumericFormat(W_TV, td::FormatFloat::Decimal, 2);
        _study.setColumnNumericFormat(W_Ms, td::FormatFloat::Decimal, 1);
    }

public:
    IterationLogView()
    : _lblSummary("Method comparison", gui::Font::ID::SystemBold)
    , _summary(td::Ownership::Extern, gui::TableEdit::RowNumberVisibility::NotVisible)
    , _lblDetail("Iterations", gui::Font::ID::SystemBold)
    , _detail(td::Ownership::Extern, gui::TableEdit::RowNumberVisibility::NotVisible)
    , _lblStudy("Lambda study", gui::Font::ID::SystemBold)
    , _study(td::Ownership::Extern, gui::TableEdit::RowNumberVisibility::NotVisible)
    , _vl(6)
    {
        initSummary();
        initDetail();
        initStudy();
        _summary.setSizeLimits(0, gui::Control::Limit::None, 90, gui::Control::Limit::Fixed);
        _study.setSizeLimits(0, gui::Control::Limit::None, 200, gui::Control::Limit::Fixed);
        _vl << _lblSummary << _summary << _lblDetail << _detail << _lblStudy << _study;
        setMargins(8, 8, 8, 8);
        setLayout(&_vl);
    }

    void setComparison(const DenoiseResult& r)
    {
        _summary.beginUpdate();
        _summary.clean();
        _detail.beginUpdate();
        _detail.clean();
        for (auto& run : r.runs)
        {
            const char* name = solverName(run.kind);
            auto& row = _summary.getEmptyRow();
            row[S_Method] = name;
            row[S_Psnr] = run.last().psnr;
            row[S_Gain] = run.last().psnr - r.noisyPsnr;
            row[S_Energy] = run.last().energy;
            row[S_Iters] = (td::INT4) run.last().k;
            row[S_Ms] = run.totalMs;
            _summary.push_back();

            for (auto& rec : run.iterations)
            {
                auto& d = _detail.getEmptyRow();
                d[D_Method] = name;
                d[D_K] = (td::INT4) rec.k;
                d[D_Energy] = rec.energy;
                d[D_Psnr] = rec.psnr;
                d[D_Rel] = rec.k == 0 ? std::numeric_limits<double>::quiet_NaN() : rec.relChange;
                d[D_Ms] = rec.timeMs;
                _detail.push_back();
            }
        }
        _summary.endUpdate();
        _detail.endUpdate();
    }

    void setStudy(const DenoiseResult& r)
    {
        _study.beginUpdate();
        _study.clean();
        for (auto& series : r.study)
            for (auto& p : series.points)
            {
                auto& row = _study.getEmptyRow();
                row[W_Method] = solverName(series.kind);
                row[W_Lambda] = p.lambda;
                row[W_Psnr] = p.psnr;
                row[W_Fid] = p.fidelity;
                row[W_TV] = p.tv;
                row[W_Ms] = p.timeMs;
                _study.push_back();
            }
        _study.endUpdate();
    }
};
