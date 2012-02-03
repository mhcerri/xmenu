#include <stdlib.h>
#include <xcb/xcb.h>
#ifdef XINERAMA
#include <xcb/xinerama.h>
#endif
#include "screen.h"

#ifdef XINERAMA
// thanks awesome
int get_xinerama_screen_geom(struct x_context *xc, struct geom *geom,
		uint16_t x, uint16_t y)
{
	/* check for extension before checking for xinerama */
	int xinerama_is_active = 0;
	if(xcb_get_extension_data(xc->conn, &xcb_xinerama_id)->present) {
		xcb_xinerama_is_active_reply_t *xia;
		xia = xcb_xinerama_is_active_reply(xc->conn,
				xcb_xinerama_is_active(xc->conn), NULL);
		xinerama_is_active = xia->state;
		free(xia);
	}

	if(!xinerama_is_active)
		return 1;

	xcb_xinerama_query_screens_reply_t *xsq;
	xcb_xinerama_screen_info_t *xsi;

	xsq = xcb_xinerama_query_screens_reply(xc->conn,
		xcb_xinerama_query_screens_unchecked(xc->conn), NULL);

	xsi = xcb_xinerama_query_screens_screen_info(xsq);
	int screen_len = xcb_xinerama_query_screens_screen_info_length(xsq);

	int i;
	for(i = 0; i < screen_len; i++) {
		if (x >= xsi[i].x_org &&
		    x <= xsi[i].x_org + xsi[i].width &&
		    y >= xsi[i].y_org &&
		    y <= xsi[i].y_org + xsi[i].height) {
			geom->x = xsi[i].x_org;
			geom->y = xsi[i].y_org;
			geom->w = xsi[i].width;
			geom->h = xsi[i].height;
			free(xsq);
			return 0;
		}

	}
	free(xsq);
	return 1;
}
#endif

int get_screen_geom(struct x_context *xc, struct geom *geom)
{
	if (!geom)
		return 1;

#ifdef XINERAMA
	/* get mouse coord */
	xcb_query_pointer_cookie_t query_ptr_c;
	xcb_query_pointer_reply_t *query_ptr_r;

	query_ptr_c = xcb_query_pointer_unchecked(xc->conn, xc->screen->root);
	query_ptr_r = xcb_query_pointer_reply(xc->conn, query_ptr_c, NULL);
	if(query_ptr_r) {
		uint16_t x, y;
		x = query_ptr_r->win_x;
		y = query_ptr_r->win_y;
		free(query_ptr_r);
		if (!get_xinerama_screen_geom(xc, geom, x, y))
			return 0;
	}
#endif

	/* fallback to X */
	geom->x = geom->y = 0;
	geom->w = xc->screen->width_in_pixels;
	geom->h = xc->screen->height_in_pixels;
	return 0;
}

