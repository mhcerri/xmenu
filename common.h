#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#define perror(fmt, ...) fprintf(stderr, fmt "\n");

struct x_context {
	xcb_connection_t *conn;
	xcb_key_symbols_t *key_symbols;
	xcb_window_t win;
	uint16_t w;
	uint16_t h;
	xcb_gcontext_t gc;
	xcb_pixmap_t pm;
	struct {
		uint32_t x;
		uint32_t y;
		uint32_t w;
		uint32_t h;
		xcb_screen_t *xscreen;
	} screen;
	struct {
		uint32_t border;
		uint32_t background;
		uint32_t foreground;
		uint32_t unique_foreground;
		uint32_t multiple_foreground;
		uint32_t sel_background;
		uint32_t sel_foreground;
	} color;
    struct {
        uint16_t width;
        uint16_t height;
        uint16_t border;
    } dimensions;
	struct {
		xcb_font_t xcb_font;
		uint16_t height;
	} font;
    struct {
        int help;
        char *font;
    } config;
};



#endif

