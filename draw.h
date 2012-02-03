/*
 * TODO: Remove unused functions.
 */
#ifndef DRAW_H
#define DRAW_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

struct x_context {
	xcb_connection_t *conn;
	xcb_gcontext_t gc;
	xcb_pixmap_t pm;
	xcb_screen_t *screen;
	xcb_key_symbols_t *key_symbols;
	struct {
		xcb_font_t xcb_font;
		uint16_t height;
	} font;
};

int init_x_context(struct x_context *xc);
void free_x_context(struct x_context *xc);
int resize_canvas(struct x_context *xc, uint16_t w, uint16_t h);
int load_font(struct x_context *xc, const char *font_name);
uint32_t get_color(struct x_context *xc, const char *hex);
void set_foreground(struct x_context *xc, uint32_t color);
void set_background(struct x_context *xc, uint32_t color);

int get_depth(struct x_context *xc);

void draw_line(struct x_context *xc, uint32_t x1, uint32_t y1,
		uint32_t x2, uint32_t y2);

uint32_t get_text_width(struct x_context *xc, const xcb_char2b_t *wstr);
uint32_t get_ntext_width(struct x_context *xc, const xcb_char2b_t *wstr,
		size_t n);

uint32_t get_text8_width(struct x_context *xc, const char *str);
uint32_t get_ntext8_width(struct x_context *xc, const char *str, size_t n);

void draw_text(struct x_context *xc, const xcb_char2b_t *wstr,
		uint32_t x, uint32_t y);
void draw_ntext(struct x_context *xc, const xcb_char2b_t *wstr,
		size_t n, uint32_t x, uint32_t y);

uint32_t draw_text_gw(struct x_context *xc, const xcb_char2b_t *wstr,
		uint32_t x, uint32_t y);
uint32_t draw_ntext_gw(struct x_context *xc, const xcb_char2b_t *wstr,
		size_t n, uint32_t x, uint32_t y);

void draw_text8(struct x_context *xc, const char *str, uint32_t x, uint32_t y);
void draw_ntext8(struct x_context *xc, const char *str, size_t n,
		uint32_t x, uint32_t y);

uint32_t draw_text8_gw(struct x_context *xc, const char *str,
		uint32_t x, uint32_t y);
uint32_t draw_ntext8_gw(struct x_context *xc, const char *str,
		size_t n, uint32_t x, uint32_t y);
#endif

