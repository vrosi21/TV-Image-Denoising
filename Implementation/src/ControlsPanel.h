#pragma once
#include <gui/View.h>
#include <gui/ComboBox.h>
#include <gui/Label.h>
#include <gui/Slider.h>
#include <gui/Button.h>
#include <gui/CheckBox.h>
#include <gui/NumericEdit.h>
#include <gui/GridLayout.h>
#include <gui/GridComposer.h>
#include "DenoisingRunner.h"
#include "PlotStyle.h"
#include <functional>
#include <random>
#include <vector>
#include <algorithm>
#include <cmath>

// ============================================================
// ControlsPanel: experiment settings sidebar.
//
//   IMAGE         built-in samples + images folder, Refresh, size info
//   NOISE         sigma, seed (+ New)
//   MODEL         lambda, Huber epsilon
//   METHODS       GD / Newton checkboxes, iterations
//   LAMBDA STUDY  range and number of points
//   Auto-run on change
//
// Every row reads  label | control | value.  Actions (Run, Lambda
// study, Stop, Export) live in the toolbar and menus, so the
// sidebar only describes *what* to compute.
//
// SRP: owns the setting widgets and reports changes; never runs
//      solvers itself.
// ============================================================
class ControlsPanel : public gui::View
{
private:
    // ---- image ------------------------------------------------
    gui::Label       _hdrImage;
    gui::ComboBox    _cmbImage;
    gui::Button      _btnOpen;
    gui::Label       _lblImageInfo;

    // ---- noise ------------------------------------------------
    gui::Label       _hdrNoise;
    gui::Label       _lblSigma;
    gui::Slider      _slSigma;
    gui::Label       _valSigma;
    gui::Label       _lblSeed;
    gui::NumericEdit _neSeed;
    gui::Button      _btnNewSeed;

    // ---- model ------------------------------------------------
    gui::Label       _hdrModel;
    gui::Label       _lblLambda;
    gui::Slider      _slLambda;
    gui::Label       _valLambda;
    gui::Label       _lblEpsilon;
    gui::Slider      _slEpsilon;
    gui::Label       _valEpsilon;

    // ---- methods ----------------------------------------------
    gui::Label       _hdrMethods;
    gui::CheckBox    _cbGD;
    gui::CheckBox    _cbNewton;
    gui::Label       _lblIter;
    gui::Slider      _slIter;
    gui::Label       _valIter;

    // ---- lambda study -----------------------------------------
    gui::Label       _hdrStudy;
    gui::Label       _lblFrom;
    gui::NumericEdit _neFrom;
    gui::Label       _lblTo;
    gui::NumericEdit _neTo;
    gui::Label       _lblPoints;
    gui::NumericEdit _nePoints;

    // ---- options ----------------------------------------------
    gui::CheckBox    _cbAuto;

    gui::GridLayout  _grid;

    std::function<void(int)>    _onSelectImage;    // 0..2 samples, 3.. user images
    std::function<void()>       _onRefreshImages;
    std::function<void()>       _onInputChanged;   // sigma / seed → new noisy preview
    std::function<void()>       _onSettingsChanged;
    std::function<void(double)> _onLambdaChanged;

    static constexpr int kSampleCount = 3;

    static void fmtValue(gui::Label& lbl, const char* format, double v)
    {
        lbl.setTitle(plot::fmt(format, v));
    }

    void inputChanged()
    {
        if (_onInputChanged) _onInputChanged();
        settingsChanged();
    }

    void settingsChanged()
    {
        if (_onSettingsChanged) _onSettingsChanged();
    }

    static double readReal(const gui::NumericEdit& ne, double lo, double hi)
    {
        double v = ne.getValue().r8Val();
        if (!std::isfinite(v)) v = lo;
        return (std::min)(hi, (std::max)(lo, v));
    }

public:
    ControlsPanel()
    : _hdrImage("IMAGE", gui::Font::ID::SystemSmallerBold)
    , _btnOpen("Refresh", "Rescan the images folder")
    , _hdrNoise("NOISE", gui::Font::ID::SystemSmallerBold)
    , _lblSigma(plot::fmt("Noise level %s", plot::glyph::sigma))
    , _slSigma("Standard deviation of the added Gaussian noise")
    , _lblSeed("Seed")
    , _neSeed(td::int4, gui::LineEdit::Messages::Send, false, "Same seed gives the same noisy image, so runs are reproducible")
    , _btnNewSeed("New", "Draw a new random seed")
    , _hdrModel("MODEL", gui::Font::ID::SystemSmallerBold)
    , _lblLambda(plot::fmt("Regularisation %s", plot::glyph::lambda))
    , _slLambda("Weight of the total-variation term: larger values smooth more")
    , _lblEpsilon(plot::fmt("Huber %s", plot::glyph::eps))
    , _slEpsilon("Smoothing of |grad u| used by Newton; smaller keeps sharper edges")
    , _hdrMethods("METHODS", gui::Font::ID::SystemSmallerBold)
    , _cbGD("Gradient descent (Armijo line search)")
    , _cbNewton("Newton method (Huber smoothing)")
    , _lblIter("Iterations")
    , _slIter("Iterations per method")
    , _hdrStudy("LAMBDA STUDY", gui::Font::ID::SystemSmallerBold)
    , _lblFrom(plot::fmt("From %s", plot::glyph::lambda))
    , _neFrom(td::real8, gui::LineEdit::Messages::Send, false, "Smallest lambda of the study")
    , _lblTo(plot::fmt("To %s", plot::glyph::lambda))
    , _neTo(td::real8, gui::LineEdit::Messages::Send, false, "Largest lambda of the study")
    , _lblPoints("Points")
    , _nePoints(td::int4, gui::LineEdit::Messages::Send, false, "Number of log-spaced lambda values")
    , _cbAuto("Run automatically when settings change")
    , _grid(18, 3)
    {
        const DenoiseSettings d;

        // ---- image ------------------------------------------------
        _cmbImage.addItem("Cameraman");
        _cmbImage.addItem("Ape");
        _cmbImage.addItem("Barbara");
        _cmbImage.selectIndex(0, false);
        _cmbImage.setToolTip("Built-in test image");
        _cmbImage.onChangedSelection([this]()
        {
            const int idx = _cmbImage.getSelectedIndex();
            if (idx >= 0 && _onSelectImage) _onSelectImage(idx);
        });
        _btnOpen.onClick([this]() { if (_onRefreshImages) _onRefreshImages(); });

        // ---- noise ------------------------------------------------
        _slSigma.setRange(0.01, 0.50);
        _slSigma.setValue(d.sigma, false);
        fmtValue(_valSigma, "%.3f", d.sigma);
        _slSigma.onChangedValue([this]() { fmtValue(_valSigma, "%.3f", _slSigma.getValue()); inputChanged(); });

        _neSeed.setValue(td::Variant((td::INT4) d.seed), false);
        _neSeed.onFinishEdit([this]() { inputChanged(); });
        _btnNewSeed.onClick([this]()
        {
            std::random_device rd;
            _neSeed.setValue(td::Variant((td::INT4) (rd() % 1000000)), false);
            inputChanged();
        });

        // ---- model ------------------------------------------------
        _slLambda.setRange(0.01, 1.00);
        _slLambda.setValue(d.lambda, false);
        fmtValue(_valLambda, "%.3f", d.lambda);
        _slLambda.onChangedValue([this]()
        {
            fmtValue(_valLambda, "%.3f", _slLambda.getValue());
            if (_onLambdaChanged) _onLambdaChanged(_slLambda.getValue());
            settingsChanged();
        });

        _slEpsilon.setRange(0.001, 0.10);
        _slEpsilon.setValue(d.epsilon, false);
        fmtValue(_valEpsilon, "%.3f", d.epsilon);
        _slEpsilon.onChangedValue([this]() { fmtValue(_valEpsilon, "%.3f", _slEpsilon.getValue()); settingsChanged(); });

        // ---- methods ----------------------------------------------
        _cbGD.setChecked(d.runGD, false);
        _cbNewton.setChecked(d.runNewton, false);
        _cbGD.setToolTip("First-order method; cheap iterations, linear convergence");
        _cbNewton.setToolTip("Second-order method; sparse linear solve per iteration, fast convergence");
        // at least one method must stay selected
        _cbGD.onClick([this]()
        {
            if (!_cbGD.isChecked() && !_cbNewton.isChecked()) _cbGD.setChecked(true, false);
            _slEpsilon.enable(_cbNewton.isChecked());
            inputChanged();
        });
        _cbNewton.onClick([this]()
        {
            if (!_cbGD.isChecked() && !_cbNewton.isChecked()) _cbNewton.setChecked(true, false);
            _slEpsilon.enable(_cbNewton.isChecked());
            inputChanged();
        });

        _slIter.setRange(1.0, 50.0);
        _slIter.setValue(d.iterations, false);
        fmtValue(_valIter, "%.0f", d.iterations);
        _slIter.onChangedValue([this]() { fmtValue(_valIter, "%.0f", std::round(_slIter.getValue())); settingsChanged(); });

        // ---- lambda study -----------------------------------------
        _neFrom.showThSep(false);
        _neFrom.setNumberOfDigitsAfterDecimalPoint(3);
        _neFrom.setValue(td::Variant(d.studyFrom), false);
        _neTo.showThSep(false);
        _neTo.setNumberOfDigitsAfterDecimalPoint(3);
        _neTo.setValue(td::Variant(d.studyTo), false);
        _nePoints.showThSep(false);
        _nePoints.setValue(td::Variant((td::INT4) d.studyPoints), false);

        _cbAuto.setToolTip("Re-run the comparison after every change (useful for exploring parameters)");
        _cbAuto.onClick([this]() { if (_cbAuto.isChecked()) settingsChanged(); });

        // ---- sizes --------------------------------------------------
        for (gui::Slider* s : { &_slSigma, &_slLambda, &_slEpsilon, &_slIter })
            s->setSizeLimits(120, gui::Control::Limit::UseAsMin);
        for (gui::Label* l : { &_valSigma, &_valLambda, &_valEpsilon, &_valIter })
            l->setSizeLimits(44, gui::Control::Limit::UseAsMin);
        for (gui::NumericEdit* n : { &_neSeed, &_neFrom, &_neTo, &_nePoints })
            n->setSizeLimits(90, gui::Control::Limit::UseAsMin);
        _cmbImage.setSizeLimits(130, gui::Control::Limit::UseAsMin);

        // ---- layout: label | control | value -----------------------
        gui::GridComposer gc(_grid);
        gc.appendRow(_hdrImage, 0);                                            //  1
        gc.appendRow(_cmbImage, 2); gc.appendCol(_btnOpen);                    //  2
        gc.appendRow(_lblImageInfo, 0);                                        //  3
        gc.appendRow(_hdrNoise, 0);                                            //  4
        gc.appendRow(_lblSigma) << _slSigma << _valSigma;                      //  5
        gc.appendRow(_lblSeed) << _neSeed << _btnNewSeed;                      //  6
        gc.appendRow(_hdrModel, 0);                                            //  7
        gc.appendRow(_lblLambda) << _slLambda << _valLambda;                   //  8
        gc.appendRow(_lblEpsilon) << _slEpsilon << _valEpsilon;                //  9
        gc.appendRow(_hdrMethods, 0);                                          // 10
        gc.appendRow(_cbGD, 0);                                                // 11
        gc.appendRow(_cbNewton, 0);                                            // 12
        gc.appendRow(_lblIter) << _slIter << _valIter;                         // 13
        gc.appendRow(_hdrStudy, 0);                                            // 14
        gc.appendRow(_lblFrom) << _neFrom;                                     // 15
        gc.appendRow(_lblTo) << _neTo;                                         // 16
        gc.appendRow(_lblPoints) << _nePoints;                                 // 17
        gc.appendRow(_cbAuto, 0);                                              // 18

        _grid.setSpaceBetweenCells(6, 8);
        setMargins(14, 12, 14, 12);
        setLayout(&_grid);
    }

    // ---- wiring -------------------------------------------------------
    void setOnSelectImage    (const std::function<void(int)>& f)    { _onSelectImage = f; }
    void setOnRefreshImages  (const std::function<void()>& f)       { _onRefreshImages = f; }
    void setOnInputChanged   (const std::function<void()>& f)       { _onInputChanged = f; }
    void setOnSettingsChanged(const std::function<void()>& f)       { _onSettingsChanged = f; }
    void setOnLambdaChanged  (const std::function<void(double)>& f) { _onLambdaChanged = f; }

    // ---- state from the coordinator ----------------------------------
    void setImageInfo(const td::String& text) { _lblImageInfo.setTitle(text); }

    // Rebuilds the list: built-in samples followed by the user images.
    // Keeps the current selection when it still exists.
    void setUserImages(const std::vector<td::String>& names)
    {
        const int selected = _cmbImage.getSelectedIndex();
        while (_cmbImage.getNoOfItems() > kSampleCount) _cmbImage.removeItem(_cmbImage.getNoOfItems() - 1);
        for (auto& n : names) _cmbImage.addItem(n);
        _cmbImage.selectIndex(selected >= 0 && selected < _cmbImage.getNoOfItems() ? selected : 0, false);
    }

    void setImagesFolderHint(const td::String& folder)
    {
        _btnOpen.setToolTip(plot::fmt("Put PNG or JPEG files into\n%s\nand press Refresh to list them here.", folder.c_str()));
        _cmbImage.setToolTip(plot::fmt("Built-in test images, followed by your images from\n%s", folder.c_str()));
    }

    bool autoRun() const { return _cbAuto.isChecked(); }

    DenoiseSettings settings() const
    {
        DenoiseSettings s;
        s.runGD       = _cbGD.isChecked();
        s.runNewton   = _cbNewton.isChecked();
        s.lambda      = static_cast<float>(_slLambda.getValue());
        s.sigma       = static_cast<float>(_slSigma.getValue());
        s.epsilon     = static_cast<float>(_slEpsilon.getValue());
        s.iterations  = static_cast<int>(std::round(_slIter.getValue()));
        s.seed        = static_cast<unsigned>((std::max)(0, _neSeed.getValue().i4Val()));
        s.studyFrom   = readReal(_neFrom, 1e-4, 100.0);
        s.studyTo     = readReal(_neTo, 1e-4, 100.0);
        if (s.studyTo <= s.studyFrom) s.studyTo = s.studyFrom * 10.0;
        s.studyPoints = (std::min)(40, (std::max)(2, _nePoints.getValue().i4Val()));
        return s;
    }
};
