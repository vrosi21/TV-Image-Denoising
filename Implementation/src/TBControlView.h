#pragma once
#include <gui/View.h>
#include <gui/Button.h>
#include <gui/Slider.h>
#include <gui/Label.h>
#include <gui/HorizontalLayout.h>
#include <functional>

// ============================================================
// TBSlider — Slider with an explicit minimum size so it
// renders correctly when embedded inside a ToolBar item.
// (Pattern from natID examples/TestGLCubeMap/TBSliderView.h)
// ============================================================
class TBSlider : public gui::Slider
{
protected:
    void getMinSize(gui::Size& minSize) const override
    {
        minSize.width  = 100;
        minSize.height = 22;
    }
public:
    TBSlider() : gui::Slider() {}
};

// ============================================================
// TBControlView — all interactive controls in one View.
//
// Embedded in the ToolBar via:
//     addItem("Controls", "tooltip", &_controlView)
//
// Layout (horizontal):
//   [Cameraman] [Ape] [Barbara]  <space>
//   λ: [====]  σ: [====]  Iter: [====]  <space>
//   [Denoise]
//
// Callbacks are set from outside (e.g. MainWindow):
//   setOnSelectImage(fn)   — called with 0/1/2 when image btn clicked
//   setOnRun(fn)           — called when Denoise button clicked
//
// SRP: owns interactive controls and their event wiring.
//      Does NOT contain business logic.
// ============================================================
class TBControlView : public gui::View
{
private:
    // Image-selection buttons
    gui::Button _btnCameraman;
    gui::Button _btnApe;
    gui::Button _btnBarbara;

    // Parameter sliders with live labels
    gui::Label  _lblLambda;
    TBSlider    _slLambda;
    gui::Label  _lblSigma;
    TBSlider    _slSigma;
    gui::Label  _lblIter;
    TBSlider    _slIter;

    // Action button
    gui::Button _btnDenoise;

    gui::HorizontalLayout _hl;

    // Coordinator callbacks
    std::function<void(int)> _onSelectImage;
    std::function<void()>    _onRun;

    // ---- helpers ------------------------------------------------
    static td::String fmtFloat(const char* prefix, double val)
    {
        td::String s;
        s.format("%s %.2f", prefix, val);
        return s;
    }
    static td::String fmtInt(const char* prefix, int val)
    {
        td::String s;
        s.format("%s %d", prefix, val);
        return s;
    }

public:
    TBControlView()
    : _btnCameraman("Cameraman")
    , _btnApe("Ape")
    , _btnBarbara("Barbara")
    , _lblLambda(fmtFloat("λ:", 0.10))
    , _lblSigma(fmtFloat("σ:", 0.10))
    , _lblIter(fmtInt("Iter:", 5))
    , _btnDenoise("Denoise")
    , _hl(10)
    {
        // Prevent labels from causing toolbar reflow as text changes
        _lblLambda.disableRemeasuring();
        _lblSigma.disableRemeasuring();
        _lblIter.disableRemeasuring();

        // Slider ranges
        _slLambda.setRange(0.01, 1.0);   _slLambda.setValue(0.10);
        _slSigma.setRange(0.01, 0.50);   _slSigma.setValue(0.10);
        _slIter.setRange(1.0, 50.0);     _slIter.setValue(5.0);

        // Update labels when sliders move
        _slLambda.onChangedValue([this]()
        {
            _lblLambda.setTitle(fmtFloat("λ:", _slLambda.getValue()));
        });
        _slSigma.onChangedValue([this]()
        {
            _lblSigma.setTitle(fmtFloat("σ:", _slSigma.getValue()));
        });
        _slIter.onChangedValue([this]()
        {
            _lblIter.setTitle(fmtInt("Iter:", static_cast<int>(_slIter.getValue())));
        });

        // Image-selection callbacks
        _btnCameraman.onClick([this]() { if (_onSelectImage) _onSelectImage(0); });
        _btnApe.onClick([this]()       { if (_onSelectImage) _onSelectImage(1); });
        _btnBarbara.onClick([this]()   { if (_onSelectImage) _onSelectImage(2); });

        // Denoise callback
        _btnDenoise.onClick([this]() { if (_onRun) _onRun(); });

        // Layout
        setMargins(0, 0, 0, 0);
        _hl.setMargins(0, 0);
        _hl.setSpaceBetweenCells(6);

        _hl << _btnCameraman << _btnApe << _btnBarbara;
        _hl.appendSpace(14);
        _hl << _lblLambda << _slLambda;
        _hl.appendSpace(8);
        _hl << _lblSigma << _slSigma;
        _hl.appendSpace(8);
        _hl << _lblIter << _slIter;
        _hl.appendSpace(14);
        _hl << _btnDenoise;

        setLayout(&_hl);
    }

    // ---- coordinator wiring ------------------------------------
    void setOnSelectImage(const std::function<void(int)>& cb) { _onSelectImage = cb; }
    void setOnRun(const std::function<void()>& cb)            { _onRun = cb; }

    // ---- value accessors for coordinator -----------------------
    float getLambda()     const { return static_cast<float>(_slLambda.getValue()); }
    float getNoiseSigma() const { return static_cast<float>(_slSigma.getValue()); }
    int   getIterations() const { return static_cast<int>(_slIter.getValue()); }
};
