#ifndef DRAW_H
#define DRAW_H

#include <xcb/xcb.h>
#include "common.h"

int get_depth(struct x_context *xc);

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

void draw_line(struct x_context *xc, uint32_t x1, uint32_t y1,
		uint32_t x2, uint32_t y2);
void set_foreground(struct x_context *xc, uint32_t color);
void set_background(struct x_context *xc, uint32_t color);
uint32_t get_color(struct x_context *xc, const char *hex);

#endif

