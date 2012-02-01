#include "draw.h"
#include <stdlib.h>
#include <string.h>
#include "util.h"

int get_depth(struct x_context *xc)
{
	xcb_drawable_t drawable = { xc->screen.xscreen->root };
	xcb_get_geometry_reply_t *geom;
	geom = xcb_get_geometry_reply(xc->conn,
			xcb_get_geometry(xc->conn, drawable), 0);
	if(!geom) {
		perror("GetGeometry(root) failed");
		abort();
	}

	int depth = geom->depth;
	free(geom);
	return depth;
}

uint32_t get_text_width(struct x_context *xc, const xcb_char2b_t *wstr)
{
	if (wstr == NULL)
		return 0;

	/* Calc lenght */
	size_t len = 0;
	for (; wstr[len].byte1 || wstr[len].byte2; len++)
		;
	return get_ntext_width(xc, wstr, len);
}

uint32_t get_ntext_width(struct x_context *xc, const xcb_char2b_t *wstr,
		size_t n)
{
	if (wstr == NULL || n == 0)
		return 0;

	/* Query about text width */
	xcb_query_text_extents_cookie_t c = xcb_query_text_extents(
	//		xc->conn, xc->font.xcb_font, len, wstr);
			xc->conn, xc->gc, n, wstr);
	xcb_query_text_extents_reply_t *r = xcb_query_text_extents_reply(
			xc->conn, c, NULL);
	if (r == NULL) {
		perror("Failed to get text information.");
		return 0;
	}
	uint32_t width = r->overall_width;
	free(r);
	return width;
}


uint32_t get_text8_width(struct x_context *xc, const char *str)
{
	if (str == NULL || str[0] == 0)
		return 0;
	xcb_char2b_t *wstr = convert_ascii_to_char2b(str);
	if (wstr == NULL)
		return 0;
	uint32_t width = get_text_width(xc, wstr);
	free(wstr);
	return width;
}

uint32_t get_ntext8_width(struct x_context *xc, const char *str, size_t n)
{
	if (str == NULL || str[0] == 0 || n == 0)
		return 0;
	xcb_char2b_t *wstr = convert_ascii_to_char2b(str);
	if (wstr == NULL)
		return 0;
	uint32_t width = get_ntext_width(xc, wstr, n);
	free(wstr);
	return width;
}

void draw_text(struct x_context *xc, const xcb_char2b_t *wstr,
		uint32_t x, uint32_t y)
{
	if (wstr) {
		size_t len = 0;
		for(; wstr[len].byte1 || wstr[len].byte2; len++)
			;
		draw_ntext(xc, wstr, len, x, y);
	}
}

void draw_ntext(struct x_context *xc, const xcb_char2b_t *wstr,
		size_t n, uint32_t x, uint32_t y)
{
	if (wstr && n) {
		xcb_image_text_16(xc->conn, n, xc->pm, xc->gc,
				x, y + xc->font.height, wstr);
	}
}

uint32_t draw_text_gw(struct x_context *xc, const xcb_char2b_t *wstr,
		uint32_t x, uint32_t y)
{
	draw_text(xc, wstr, x, y);
	return get_text_width(xc, wstr);
}

uint32_t draw_ntext_gw(struct x_context *xc, const xcb_char2b_t *wstr,
		size_t n, uint32_t x, uint32_t y)
{
	draw_ntext(xc, wstr, n, x, y);
	return get_ntext_width(xc, wstr, n);
}

void draw_text8(struct x_context *xc, const char *str, uint32_t x, uint32_t y)
{
	xcb_char2b_t *wstr = convert_ascii_to_char2b(str);
	draw_text(xc, wstr, x, y);
	free(wstr);
}

void draw_ntext8(struct x_context *xc, const char *str, size_t n,
		uint32_t x, uint32_t y) {
	xcb_char2b_t *wstr = convert_ascii_to_char2b(str);
	draw_ntext(xc, wstr, n, x, y);
	free(wstr);
}

uint32_t draw_text8_gw(struct x_context *xc, const char *str,
		uint32_t x, uint32_t y)
{
	xcb_char2b_t *wstr = convert_ascii_to_char2b(str);
	uint32_t w = draw_text_gw(xc, wstr, x, y);
	free(wstr);
	return w;
}

uint32_t draw_ntext8_gw(struct x_context *xc, const char *str,
		size_t n, uint32_t x, uint32_t y)
{
	xcb_char2b_t *wstr = convert_ascii_to_char2b(str);
	uint32_t w = draw_ntext_gw(xc, wstr, n, x, y);
	free(wstr);
	return w;
}

void draw_line(struct x_context *xc, uint32_t x1, uint32_t y1,
		uint32_t x2, uint32_t y2)
{
	xcb_void_cookie_t c;
	xcb_segment_t segm = { x1, y1, x2, y2 };

	c = xcb_poly_segment(xc->conn,   /* connection to X server */
		xc->pm,                  /* drawable               */
		xc->gc,                  /* graphic Context        */
		1,                       /* the number of points   */
		&segm);                  /* array of segments.     */
}

void set_foreground(struct x_context *xc, uint32_t color)
{
	xcb_change_gc(xc->conn, xc->gc, XCB_GC_FOREGROUND, &color);
}

void set_background(struct x_context *xc, uint32_t color)
{
	xcb_change_gc(xc->conn, xc->gc, XCB_GC_BACKGROUND, &color);
}

uint32_t get_color(struct x_context *xc, const char *hex)
{
	// TODO improve it
	// TODO handler error
	int i;
	const size_t len = 7;
	if (hex == NULL || strlen(hex) != len || *hex != '#') {
		perror("Invalid color");
		exit(-1);
	}
	for (i = 1; i < len; i++) {
		if (!isxdigit(hex[i])) {
			perror("Invalid color");
			exit(-1);
		}
	}

	/* from libi3 */
	char strgroups[3][3] = {{hex[1], hex[2], '\0'},
	                        {hex[3], hex[4], '\0'},
	                        {hex[5], hex[6], '\0'}};
	uint16_t r = strtol(strgroups[0], NULL, 16);
	uint16_t g = strtol(strgroups[1], NULL, 16);
	uint16_t b = strtol(strgroups[2], NULL, 16);

	/* Alocate color */
	xcb_alloc_color_cookie_t color_c = xcb_alloc_color(
			xc->conn, xc->screen.xscreen->default_colormap,
			255*r, 255*g, 255*b);
	xcb_alloc_color_reply_t *color_r = xcb_alloc_color_reply(
			xc->conn, color_c, NULL);
	if (color_r == NULL) {
		perror("Failed to allocate color\n");
		exit(-1);
	}
	uint32_t color_pixel = color_r->pixel;
	free(color_r);
	return color_pixel;
}

