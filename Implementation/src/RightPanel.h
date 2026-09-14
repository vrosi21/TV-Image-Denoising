#pragma once
#include <gui/View.h>
#include <gui/SplitterLayout.h>
#include <gui/TextEdit.h>
#include "ControlsPanel.h"
#include <functional>

// ============================================================
// RightPanel — the auxiliary (right) cell of the main window.
//
// Splits vertically into two zones:
//
//   ┌─────────────────┐
//   │  ControlsPanel  │  ← AuxiliaryCell::First (top)
//   │                 │    starts at its natural content height
//   ├─────────────────┤  ← user-draggable divider
//   │  Log (TextEdit) │  ← main cell (bottom) fills remaining space
//   │                 │    minimum 200 px so it's never a sliver
//   └─────────────────┘
//
// Using AuxiliaryCell::First means the CONTROLS are the auxiliary
// (pinned to their measured content height) and the LOG is the
// main cell that fills whatever is left — giving a roughly equal
// split at typical window heights without needing a private
// _sizeOfAuxiliaryCell setter (which the framework doesn't expose).
//
// After each Denoise run the log is cleared and repopulated
// with algorithm name, parameters, and F(u_k) per iteration.
//
// SRP: owns the visual split between controls and log only.
//      Control logic lives in ControlsPanel.
//      Log content is written by DenoisingPanel.
// ============================================================
class RightPanel : public gui::View
{
private:
    gui::SplitterLayout _sl;
    ControlsPanel       _controls;
    gui::TextEdit       _log;

public:
    RightPanel()
    : _sl(gui::SplitterLayout::Orientation::Vertical,
          gui::SplitterLayout::AuxiliaryCell::First)   // controls=aux(top), log=main(bottom)
    // read-only TextEdit, no horizontal scroll, no change events
    , _log(gui::TextEdit::HorizontalScroll::No,
           gui::TextEdit::Events::DoNotSend,
           /*readOnly=*/true)
    {
        // Give the log a minimum height so it never collapses to a sliver.
        // The splitter bar is user-draggable from this point upward.
        _log.setSizeLimits(0, gui::Control::Limit::None,
                           200, gui::Control::Limit::UseAsMin);

        setMargins(0, 0, 0, 0);
        _sl.setMargins(0, 0);
        // controls = cell-1 = AuxiliaryCell::First (top, natural height)
        // log      = cell-2 = main cell           (bottom, fills rest)
        _sl.setContent(_controls, _log);
        setLayout(&_sl);
    }

    // ---- ControlsPanel callback wiring ----------------------
    void setOnSelectAlgo (const std::function<void(int)>&         cb) { _controls.setOnSelectAlgo(cb);  }
    void setOnSelectImage(const std::function<void(int)>&         cb) { _controls.setOnSelectImage(cb); }
    void setOnBrowse     (const std::function<void(const char*)>& cb) { _controls.setOnBrowse(cb);      }
    void setOnRun        (const std::function<void()>&            cb) { _controls.setOnRun(cb);          }

    // ---- ControlsPanel value accessors ----------------------
    int   getAlgorithm()  const { return _controls.getAlgorithm();  }
    float getLambda()     const { return _controls.getLambda();     }
    float getNoiseSigma() const { return _controls.getNoiseSigma(); }
    int   getIterations() const { return _controls.getIterations(); }
    float getEpsilon()    const { return _controls.getEpsilon();    }

    // ---- Log access -----------------------------------------
    // Append a line of text to the log view.
    void appendLog(const char* text) { _log.appendString(text); }

    // Clear the log — called at the start of each Denoise run.
    void clearLog() { _log.clean(); }
};
