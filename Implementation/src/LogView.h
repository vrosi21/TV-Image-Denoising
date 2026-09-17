#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/Button.h>
#include <gui/TextEdit.h>
#include <gui/HorizontalLayout.h>
#include <gui/VerticalLayout.h>
#include <td/Time.h>
#include <cstdarg>
#include <cstdio>

// ============================================================
// LogView: activity log page.
// Keeps a timestamped history of every run in the session
// (parameters and per-method results) that can be copied into
// a report.  Entries accumulate until the user clears the log.
// ============================================================
class LogView : public gui::View
{
    gui::Label            _title;
    gui::Button           _btnClear;
    gui::HorizontalLayout _hl;
    gui::TextEdit         _text;
    gui::VerticalLayout   _vl;

public:
    LogView()
    : _title("Activity log", gui::Font::ID::SystemBold)
    , _btnClear("Clear log", "Remove all entries")
    , _hl(3)
    , _text(gui::TextEdit::HorizontalScroll::No, gui::TextEdit::Events::DoNotSend, /*readOnly=*/true)
    , _vl(2)
    {
        _btnClear.onClick([this]() { _text.clean(); });
        _hl << _title;
        _hl.appendSpacer();
        _hl << _btnClear;
        _vl << _hl << _text;
        setMargins(8, 8, 8, 8);
        setLayout(&_vl);
    }

    // Appends one line prefixed with the current time.
    void entry(const char* fmt, ...)
#ifdef __GNUC__
        __attribute__((format(printf, 2, 3)))
#endif
    {
        char msg[512];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(msg, sizeof msg, fmt, args);
        va_end(args);

        td::Time now(true);
        char line[600];
        std::snprintf(line, sizeof line, "[%02d:%02d:%02d]  %s\n", now.getHour(), now.getMinute(), now.getSecond(), msg);
        _text.appendString(line);
    }

    // Appends an indented continuation line without a timestamp.
    void detail(const char* fmt, ...)
#ifdef __GNUC__
        __attribute__((format(printf, 2, 3)))
#endif
    {
        char msg[512];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(msg, sizeof msg, fmt, args);
        va_end(args);

        char line[600];
        std::snprintf(line, sizeof line, "            %s\n", msg);
        _text.appendString(line);
    }
};
