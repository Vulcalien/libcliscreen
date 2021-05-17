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
#include "private/scrbuffer.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef __unix__
    #include <unistd.h>
#elif _WIN32
    #include <windows.h>
    #include "private/terminal.h"
#endif

#define PRINTF_TMP_SIZE (256)

/* Check if the buffer has enought space. If not, expand it. */
static void check_buffer(struct scrbuffer *buf, u32 requested_space);

extern void screen_scrbuffer_putc(struct scrbuffer *buf, char chr) {
    check_buffer(buf, 1);
    buf->chr_buf[buf->used] = chr;
    buf->used++;
}

void screen_scrbuffer_puts(struct scrbuffer *buf, const char *str) {
    u32 len = strlen(str);

    check_buffer(buf, len);
    for(u32 i = 0; i < len; i++) {
        buf->chr_buf[buf->used + i] = str[i];
    }
    buf->used += len;
}

void screen_scrbuffer_printf(struct scrbuffer *buf,
                             const char *format, ...) {
    va_list args;
    va_start(args, format);

    char tmp[PRINTF_TMP_SIZE] = {0};
    vsnprintf(tmp, PRINTF_TMP_SIZE, format, args);

    va_end(args);

    screen_scrbuffer_puts(buf, tmp);
}

void screen_scrbuffer_flush(struct scrbuffer *buf) {
    #ifdef __unix__
        write(STDOUT_FILENO, buf->chr_buf, buf->used);
    #elif _WIN32
        DWORD written;
        WriteConsoleA(h_out, buf->chr_buf, buf->used, &written, NULL);
    #endif

    buf->used = 0;
}

static void check_buffer(struct scrbuffer *buf, u32 requested_space) {
    // XXX this is probably broken

    // TODO optimize, using a while here is not necessary
    // instead of reallocating many times, find out how much you need to
    // expand, instead of increasing size by a 1 inc_step at a time
    while(buf->used + requested_space > buf->size) {
        buf->size += buf->inc_step;
        buf->chr_buf = realloc(buf->chr_buf, buf->size * sizeof(char));
    }
}
