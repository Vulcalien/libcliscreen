/* Copyright 2021 Vulcalien
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 only.
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
#include "screen.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "private/scrbuffer.h"
#include "private/terminal.h"

#define PRINTF_TMP_SIZE (256)

struct screen {
    u32 w;
    u32 h;

    u32 raster_size;

    u32 align_x;
    u32 align_y;

    char *raster;
    const char **colors;

    struct scrbuffer *buf;

    char ignored_char;
    bool should_clear_term;
};

static void screen_free_memory(void);

static struct screen *scr = NULL;

static struct terminal_size last_term_size = { .w = 0, .h = 0 };

EXPORT void screen_create(void) {
    // calloc sets everything to 0 or (if pointer) NULL
    scr = calloc(1, sizeof(struct screen));

    scr->align_x = SCREEN_ALIGN_X_CENTER;
    scr->align_y = SCREEN_ALIGN_Y_MIDDLE;
}

static void screen_free_memory(void) {
    if(scr->raster) free(scr->raster);
    if(scr->colors) free(scr->colors);
    if(scr->buf)    scrbuffer_destroy(&scr->buf);
}

EXPORT void screen_destroy(void) {
    if(scr) {
        screen_free_memory();
        free(scr);
    }
}

EXPORT void screen_setsize(u32 w, u32 h) {
    screen_free_memory();

    u32 raster_size = w * h;

    scr->w = w;
    scr->h = h;

    scr->raster_size = raster_size;

    scr->raster = calloc(raster_size, sizeof(char));
    scr->colors = calloc(raster_size, sizeof(const char *));

    scr->buf = scrbuffer_create(raster_size);

    scr->should_clear_term = true;
}

EXPORT void screen_setalign(u32 align_x, u32 align_y) {
    scr->align_x = align_x;
    scr->align_y = align_y;

    scr->should_clear_term = true;
}

EXPORT void screen_render(void) {
    struct terminal_size term_size = screen_terminal_size();

    // if the terminal dimension changed, clear the terminal
    if(term_size.w != last_term_size.w
       || term_size.h != last_term_size.h) {
        last_term_size = term_size;

        scr->should_clear_term = true;
    }

    if(scr->should_clear_term) {
        scr->should_clear_term = false;

        // "\033[H" - move to top left corner
        // "\033[J" - clear (delete from cursor to end of screen)
        scrbuffer_puts(scr->buf, "\033[H" "\033[J");
    }

    const char *last_color = NULL;

    u32 x0;
    switch(scr->align_x) {
        case SCREEN_ALIGN_X_LEFT:
            x0 = 1;
            break;
        case SCREEN_ALIGN_X_CENTER:
            x0 = (term_size.w - scr->w) / 2;
            break;
        case SCREEN_ALIGN_X_RIGHT:
            x0 = term_size.w - scr->w;
            break;
    }

    u32 y0;
    switch(scr->align_y) {
        case SCREEN_ALIGN_Y_TOP:
            y0 = 1;
            break;
        case SCREEN_ALIGN_Y_MIDDLE:
            y0 = (term_size.h - scr->h) / 2;
            break;
        case SCREEN_ALIGN_Y_BOTTOM:
            y0 = term_size.h - scr->h;
            break;
    }

    for(u32 y = 0; y < scr->h; y++) {
        scrbuffer_printf(scr->buf, "\033[%d;%dH", y0 + y, x0);

        for(u32 x = 0; x < scr->w; x++) {
            u32 i = x + y * scr->w;

            char chr = scr->raster[i];
            const char *col = scr->colors[i];

            // if color is different, reset and print the new one
            if(col != last_color) {
                // "\033[m" - reset color
                scrbuffer_puts(scr->buf, "\033[m");
                if(col != NULL)
                    scrbuffer_puts(scr->buf, col);

                last_color = col;
            }

            scrbuffer_putc(scr->buf, chr);
        }
    }
    if(last_color != NULL) {
        // "\033[m" - reset color
        scrbuffer_puts(scr->buf, "\033[m");
    }
    scrbuffer_flush(scr->buf);
}

EXPORT void screen_ignored_char(char c) {
    scr->ignored_char = c;
}

EXPORT void screen_clear(char c, const char *color) {
    for(u32 i = 0; i < scr->raster_size; i++) {
        scr->raster[i] = c;
        scr->colors[i] = color;
    }
}

EXPORT void screen_setchar(u32 x, u32 y, char c, const char *color) {
    if(x >= scr->w) return;
    if(y >= scr->h) return;

    scr->raster[x + y * scr->w] = c;
    scr->colors[x + y * scr->w] = color;
}

EXPORT void screen_puts(u32 x, u32 y,
                        const char *str, const char *color) {
    u32 len = strlen(str);

    u32 xoff = 0;
    u32 yoff = 0;
    for(u32 i = 0; i < len; i++) {
        char c = str[i];

        if(c == '\n') {
            xoff = 0;
            yoff++;
        } else {
            if(c != scr->ignored_char)
                screen_setchar(x + xoff, y + yoff, c, color);
            xoff++;
        }
    }
}

EXPORT void screen_printf(u32 x, u32 y,
                          const char *color,
                          const char *format, ...) {
    va_list args;
    va_start(args, format);

    char tmp[PRINTF_TMP_SIZE] = {0};
    vsnprintf(tmp, PRINTF_TMP_SIZE, format, args);

    va_end(args);

    screen_puts(x, y, tmp, color);
}
