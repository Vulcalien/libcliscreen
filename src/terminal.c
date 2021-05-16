/* Copyright 2021 Vulcalien
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <stdio.h>

#include "private/terminal.h"

static void terminal_prepare_os(void);
static void terminal_reset_os(void);

void screen_terminal_prepare(void) {
    // on Windows this activates ANSI codes,
    // so this must be done at the beginning
    terminal_prepare_os();

    fputs("\033[?25l", stdout); // hide cursor
    fputs("\033[2J",   stdout); // clear (move content up)
}

void screen_terminal_reset(void) {
    fputs("\033[?25h", stdout); // show cursor
    fputs("\033[m",    stdout); // reset color

    // on Windows this usually disables ANSI codes,
    // so this must be done at the end
    terminal_reset_os();
}

#ifdef __unix__
    #include <unistd.h>
    #include <termios.h>
    #include <sys/ioctl.h>

    static struct termios old;

    static void terminal_prepare_os(void) {
        tcgetattr(STDIN_FILENO, &old);

        struct termios new = old;

        new.c_lflag &= ~ICANON;
        new.c_lflag &= ~ECHO;

        tcsetattr(STDIN_FILENO, TCSANOW, &new);
    }

    static void terminal_reset_os(void) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
    }

    struct screen_terminal_size screen_terminal_size(void) {
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        return (struct screen_terminal_size) {ws.ws_col, ws.ws_row};
    }

#elif _WIN32
    #include <windows.h>

    static HANDLE h_in = NULL;
    static HANDLE h_out = NULL;

    static DWORD old_out;
    static DWORD old_in;

    static void terminal_prepare_os(void) {
        h_in  = GetStdHandle(STD_INPUT_HANDLE);
        h_out = GetStdHandle(STD_OUTPUT_HANDLE);

        DWORD new_in  = 0;
        DWORD new_out = 0;

        GetConsoleMode(h_in,  &new_in);
        GetConsoleMode(h_out, &new_out);

        old_in  = new_in;
        old_out = new_out;

        new_in &= ~ENABLE_LINE_INPUT;
        new_in &= ~ENABLE_ECHO_INPUT;

        new_out |= ENABLE_VIRTUAL_TERMINAL_PROCESSING; // ANSI codes

        SetConsoleMode(h_in,  new_in);
        SetConsoleMode(h_out, new_out);
    }

    static void terminal_reset_os(void) {
        SetConsoleMode(h_in, old_in);
        SetConsoleMode(h_out, old_out);
    }

    struct screen_terminal_size screen_terminal_size(void) {
        CONSOLE_SCREEN_BUFFER_INFO buf_info;
        GetConsoleScreenBufferInfo(h_out, &buf_info);
        return (struct screen_terminal_size) {
            buf_info.srWindow.Right - buf_info.srWindow.Left + 1,
            buf_info.srWindow.Bottom - buf_info.srWindow.Top + 1
        };
    }
#endif
