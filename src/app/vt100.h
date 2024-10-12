#ifndef VT100_H
#define VT100_H

// Set ESC escape codes
#define VT100_ESC "\033"

// Screen clearing and reset
#define VT100_CLEAR_SCREEN VT100_ESC "[2J"
#define VT100_RESET        VT100_ESC "c"

// Cursor positioning
#define VT100_CURSOR_HOME          VT100_ESC "[H"
#define VT100_CURSOR_POS(row, col) VT100_ESC "[" #row ";" #col "H"
#define VT100_CURSOR_UP(n)         VT100_ESC "[" #n "A"
#define VT100_CURSOR_DOWN(n)       VT100_ESC "[" #n "B"
#define VT100_CURSOR_RIGHT(n)      VT100_ESC "[" #n "C"
#define VT100_CURSOR_LEFT(n)       VT100_ESC "[" #n "D"
#define VT100_CURSOR_END           VT100_ESC "[F"

// Save and restore cursor position
#define VT100_SAVE_CURSOR    VT100_ESC "[s"
#define VT100_RESTORE_CURSOR VT100_ESC "[u"

// Hide and show cursor
#define VT100_HIDE_CURSOR VT100_ESC "[?25l"
#define VT100_SHOW_CURSOR VT100_ESC "[?25h"

// Line clearing
#define VT100_CLEAR_LINE             VT100_ESC "[2K"
#define VT100_CLEAR_LINE_FROM_CURSOR VT100_ESC "[K"
#define VT100_CLEAR_LINE_TO_CURSOR   VT100_ESC "[1K"

// Screen clearing from cursor
#define VT100_CLEAR_SCREEN_FROM_CURSOR VT100_ESC "[J"
#define VT100_CLEAR_SCREEN_TO_CURSOR   VT100_ESC "[1J"

// Text colors
#define VT100_COLOR_RESET   VT100_ESC "[0m"
#define VT100_COLOR_BLACK   VT100_ESC "[30m"
#define VT100_COLOR_RED     VT100_ESC "[31m"
#define VT100_COLOR_GREEN   VT100_ESC "[32m"
#define VT100_COLOR_YELLOW  VT100_ESC "[33m"
#define VT100_COLOR_BLUE    VT100_ESC "[34m"
#define VT100_COLOR_MAGENTA VT100_ESC "[35m"
#define VT100_COLOR_CYAN    VT100_ESC "[36m"
#define VT100_COLOR_WHITE   VT100_ESC "[37m"

// Background colors
#define VT100_BG_COLOR_BLACK   VT100_ESC "[40m"
#define VT100_BG_COLOR_RED     VT100_ESC "[41m"
#define VT100_BG_COLOR_GREEN   VT100_ESC "[42m"
#define VT100_BG_COLOR_YELLOW  VT100_ESC "[43m"
#define VT100_BG_COLOR_BLUE    VT100_ESC "[44m"
#define VT100_BG_COLOR_MAGENTA VT100_ESC "[45m"
#define VT100_BG_COLOR_CYAN    VT100_ESC "[46m"
#define VT100_BG_COLOR_WHITE   VT100_ESC "[47m"

// Text styles
#define VT100_TEXT_BOLD      VT100_ESC "[1m"
#define VT100_TEXT_UNDERLINE VT100_ESC "[4m"
#define VT100_TEXT_BLINK     VT100_ESC "[5m"
#define VT100_TEXT_REVERSE   VT100_ESC "[7m"
#define VT100_TEXT_CONCEALED VT100_ESC "[8m"

#endif // VT100_H
