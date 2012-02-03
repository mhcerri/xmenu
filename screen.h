#ifndef SCREEN_H
#define SCREEN_H

#include <xcb/xcb.h>
#include "draw.h"

struct geom {
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
};

int get_screen_geom(struct x_context *xc, struct geom *geom);

#endif

