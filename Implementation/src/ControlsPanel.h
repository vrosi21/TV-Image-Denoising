#pragma once
#include <gui/View.h>
#include <gui/ComboBox.h>
#include <gui/Label.h>
#include <gui/Slider.h>
#include <gui/Button.h>
#include <gui/VerticalLayout.h>
#include <gui/FileDialog.h>
#include <functional>

// ============================================================
// ControlsPanel — right-hand side panel with all user controls.
//
// Layout (vertical, 20 cells):
//   [Label]   Algorithm:
//   [ComboBox] GD Backtracking | Newton-Huber
//   [space]
//   [Label]   Image:
//   [ComboBox] Cameraman / Ape / Barbara
//   [Button]  Browse…                    ← opens native file dialog
//   [space]
//   [Label]   λ (regularisation weight)
//   [Slider]  lambda 0.01 – 1.0
//   [space]
//   [Label]   σ (noise sigma)
//   [Slider]  sigma 0.01 – 0.50
//   [space]
//   [Label]   Iterations
//   [Slider]  iterations 1 – 50
//   [space]
//   [Label]   ε (Huber, Newton only)   ← greyed when GD selected
//   [Slider]  epsilon 0.001 – 0.1      ← greyed when GD selected
//   [space]
//   [Button]  Denoise
//
// SRP: owns control widgets and their event wiring only.
// ============================================================
class ControlsPanel : public gui::View
{
private:
    // Unique ID for the open-file dialog (avoids double-opening)
    enum { kDlgBrowseImage = 501 };

    // ---- algorithm selection --------------------------------
    gui::Label    _lblAlgo;
    gui::ComboBox _cmbAlgo;

    // ---- image selection ------------------------------------
    gui::Label    _lblImage;
    gui::ComboBox _cmbImage;
    gui::Button   _btnBrowse;   // opens native file picker

    // ---- shared parameters ----------------------------------
    gui::Label    _lblLambda;
    gui::Slider   _slLambda;

    gui::Label    _lblSigma;
    gui::Slider   _slSigma;

    gui::Label    _lblIter;
    gui::Slider   _slIter;

    // ---- Newton-only parameter ------------------------------
    gui::Label    _lblEpsilon;
    gui::Slider   _slEpsilon;

    gui::Button   _btnDenoise;

    gui::VerticalLayout _vl;

    // ---- callbacks ------------------------------------------
    std::function<void(int)>        _onSelectAlgo;
    std::function<void(int)>        _onSelectImage;
    std::function<void(const char*)> _onBrowse;
    std::function<void()>           _onRun;

    // ---- helpers --------------------------------------------
    static td::String fmtFloat(const char* pfx, double v)
    {
        td::String s; s.format("%s %.3f", pfx, v); return s;
    }
    static td::String fmtInt(const char* pfx, int v)
    {
        td::String s; s.format("%s %d", pfx, v); return s;
    }

    void updateEpsilonState(int algoIdx)
    {
        bool newton = (algoIdx == 1);
        _lblEpsilon.enable(newton);
        _slEpsilon.enable(newton);
    }

public:
    ControlsPanel()
    : _lblAlgo("Algorithm:")
    , _lblImage("Image:")
    , _btnBrowse("Browse\xe2\x80\xa6")  // "Browse…" in UTF-8
    , _lblLambda(fmtFloat("lambda (regularisation):", 0.100))
    , _lblSigma(fmtFloat("sigma (noise level):", 0.100))
    , _lblIter(fmtInt("Iterations:", 10))
    , _lblEpsilon(fmtFloat("epsilon (Huber, Newton only):", 0.010))
    , _btnDenoise("Denoise")
    , _vl(20)   // 20 cells: see layout above
    {
        // ---- algorithm dropdown -----------------------------
        _cmbAlgo.addItem("GD Backtracking");
        _cmbAlgo.addItem("Newton-Huber");
        _cmbAlgo.selectIndex(0, false);

        // ---- image dropdown ---------------------------------
        _cmbImage.addItem("Cameraman");
        _cmbImage.addItem("Ape");
        _cmbImage.addItem("Barbara");
        _cmbImage.selectIndex(0, false);

        // ---- Browse button — opens native OS file dialog ----
        // gui::View IS-A gui::Frame, so 'this' is valid as the parent.
        _btnBrowse.onClick([this]()
        {
            gui::OpenFileDialog::show(
                this,
                td::String("Open Image"),
                {
                    {td::String("PNG Image"),  "*.png" },
                    {td::String("JPEG Image"), "*.jpg" },
                    {td::String("JPEG Image"), "*.jpeg"}
                },
                kDlgBrowseImage,
                [this](gui::FileDialog* pDlg)
                {
                    if (pDlg->getStatus() == gui::FileDialog::Status::OK)
                    {
                        td::String path = pDlg->getFileName();
                        if (!path.isEmpty() && _onBrowse)
                            _onBrowse(path.c_str());
                    }
                }
            );
        });

        // ---- slider ranges and defaults ---------------------
        _slLambda.setRange(0.01, 1.00);   _slLambda.setValue(0.10);
        _slSigma.setRange(0.01, 0.50);    _slSigma.setValue(0.10);
        _slIter.setRange(1.0, 50.0);      _slIter.setValue(10.0);
        _slEpsilon.setRange(0.001, 0.10); _slEpsilon.setValue(0.010);

        // ---- live label updates -----------------------------
        _slLambda.onChangedValue([this]() {
            _lblLambda.setTitle(fmtFloat("lambda (regularisation):", _slLambda.getValue()));
        });
        _slSigma.onChangedValue([this]() {
            _lblSigma.setTitle(fmtFloat("sigma (noise level):", _slSigma.getValue()));
        });
        _slIter.onChangedValue([this]() {
            _lblIter.setTitle(fmtInt("Iterations:", static_cast<int>(_slIter.getValue())));
        });
        _slEpsilon.onChangedValue([this]() {
            _lblEpsilon.setTitle(fmtFloat("epsilon (Huber, Newton only):", _slEpsilon.getValue()));
        });

        // ---- algorithm selection ----------------------------
        _cmbAlgo.onChangedSelection([this]() {
            int idx = _cmbAlgo.getSelectedIndex();
            updateEpsilonState(idx);
            if (_onSelectAlgo) _onSelectAlgo(idx);
        });

        // ---- image selection --------------------------------
        _cmbImage.onChangedSelection([this]() {
            int idx = _cmbImage.getSelectedIndex();
            if (idx >= 0 && _onSelectImage) _onSelectImage(idx);
        });

        // ---- denoise button ---------------------------------
        _btnDenoise.onClick([this]() { if (_onRun) _onRun(); });

        // ---- minimum widths ---------------------------------
        _cmbAlgo.setSizeLimits(220, gui::Control::Limit::UseAsMin);
        _cmbImage.setSizeLimits(220, gui::Control::Limit::UseAsMin);
        _btnBrowse.setSizeLimits(220, gui::Control::Limit::UseAsMin);
        _slLambda.setSizeLimits(220, gui::Control::Limit::UseAsMin);
        _slSigma.setSizeLimits(220, gui::Control::Limit::UseAsMin);
        _slIter.setSizeLimits(220, gui::Control::Limit::UseAsMin);
        _slEpsilon.setSizeLimits(220, gui::Control::Limit::UseAsMin);
        _btnDenoise.setSizeLimits(220, gui::Control::Limit::UseAsMin);

        // ---- layout -----------------------------------------
        setMargins(12, 12, 12, 12);
        _vl.setSpaceBetweenCells(8);

        _vl.append(_lblAlgo);           // 1
        _vl.append(_cmbAlgo);           // 2
        _vl.appendSpace(8);             // 3
        _vl.append(_lblImage);          // 4
        _vl.append(_cmbImage);          // 5
        _vl.append(_btnBrowse);         // 6  ← Browse custom image
        _vl.appendSpace(8);             // 7
        _vl.append(_lblLambda);         // 8
        _vl.append(_slLambda);          // 9
        _vl.appendSpace(4);             // 10
        _vl.append(_lblSigma);          // 11
        _vl.append(_slSigma);           // 12
        _vl.appendSpace(4);             // 13
        _vl.append(_lblIter);           // 14
        _vl.append(_slIter);            // 15
        _vl.appendSpace(4);             // 16
        _vl.append(_lblEpsilon);        // 17
        _vl.append(_slEpsilon);         // 18
        _vl.appendSpace(12);            // 19
        _vl.append(_btnDenoise);        // 20

        setLayout(&_vl);

        // GD selected by default — disable Newton-only controls
        updateEpsilonState(0);
    }

    // ---- coordinator wiring ---------------------------------
    void setOnSelectAlgo (const std::function<void(int)>&         cb) { _onSelectAlgo  = cb; }
    void setOnSelectImage(const std::function<void(int)>&         cb) { _onSelectImage = cb; }
    void setOnBrowse     (const std::function<void(const char*)>& cb) { _onBrowse      = cb; }
    void setOnRun        (const std::function<void()>&            cb) { _onRun         = cb; }

    // ---- value accessors ------------------------------------
    int   getAlgorithm()  const { return _cmbAlgo.getSelectedIndex(); }
    float getLambda()     const { return static_cast<float>(_slLambda.getValue()); }
    float getNoiseSigma() const { return static_cast<float>(_slSigma.getValue()); }
    int   getIterations() const { return static_cast<int>(_slIter.getValue()); }
    float getEpsilon()    const { return static_cast<float>(_slEpsilon.getValue()); }
};
