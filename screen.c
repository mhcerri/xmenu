#include <xcb/xcb.h>
#include <xcb/xinerama.h>
#include <xcb/randr.h>
#include "screen.h"

// thanks awesome

int scan_x_screens(struct x_context *xc)
{
	/* get information about the screen */
	const xcb_setup_t *setup = xcb_get_setup(xc->conn);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	xc->screen.xscreen = iter.data;
	xc->screen.x = xc->screen.y = 0;
	xc->screen.w = xc->screen.xscreen->width_in_pixels;
	xc->screen.h = xc->screen.xscreen->height_in_pixels;
	return 0;
}

int scan_xinerama_screens(struct x_context *xc, uint16_t x, uint16_t y)
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
			xc->screen.x = xsi[i].x_org;
			xc->screen.y = xsi[i].y_org;
			xc->screen.w = xsi[i].width;
			xc->screen.h = xsi[i].height;
		}

	}
	free(xsq);
	return 0;
}

int scan_randr_screens(struct x_context *xc, uint16_t x, uint16_t y)
{
	// TODO randr is slow... why?!
	int randr_is_active = 0;
	if(xcb_get_extension_data(xc->conn, &xcb_randr_id)->present) {
		/* check randr version */
		xcb_randr_query_version_reply_t *version_reply;
		version_reply = xcb_randr_query_version_reply(xc->conn,
				xcb_randr_query_version(xc->conn, 1, 1), 0);
		if(version_reply)
			randr_is_active = 1;
		free(version_reply);
	}

	if (!randr_is_active)
		return 1;

	xcb_randr_get_screen_resources_cookie_t screen_res_c;
	xcb_randr_get_screen_resources_reply_t *screen_res_r;
	screen_res_c = xcb_randr_get_screen_resources(xc->conn,
			xc->screen.xscreen->root);
	screen_res_r = xcb_randr_get_screen_resources_reply(xc->conn,
			screen_res_c, NULL);

	if (screen_res_r->num_crtcs <= 1) {
		free(screen_res_r);
		return 1;
	}

	/* loop through screens */
	int i;
	xcb_randr_crtc_t *randr_crtcs;
	randr_crtcs = xcb_randr_get_screen_resources_crtcs(screen_res_r);
	for(i = 0; i < screen_res_r->num_crtcs; i++) {
		xcb_randr_get_crtc_info_cookie_t crtc_info_c;
		xcb_randr_get_crtc_info_reply_t *crtc_info_r;
		crtc_info_c = xcb_randr_get_crtc_info(xc->conn, randr_crtcs[i],
				XCB_CURRENT_TIME);
		crtc_info_r = xcb_randr_get_crtc_info_reply(xc->conn,
				crtc_info_c, NULL);

		if(xcb_randr_get_crtc_info_outputs_length(crtc_info_r)) {
			if (x >= crtc_info_r->x &&
			    x <= crtc_info_r->x + crtc_info_r->width &&
			    y >= crtc_info_r->y &&
			    y <= crtc_info_r->y + crtc_info_r->height) {
				xc->screen.x = crtc_info_r->x;
				xc->screen.y = crtc_info_r->y;
				xc->screen.w = crtc_info_r->width;
				xc->screen.h = crtc_info_r->height;
			}
		}
		free(crtc_info_r);
	}
	free(screen_res_r);
	return 0;
}

int scan_screens(struct x_context *xc)
{
	if (scan_x_screens(xc))
		return 1;

	/* get mouse coord */
	xcb_query_pointer_cookie_t query_ptr_c;
	xcb_query_pointer_reply_t *query_ptr_r;

	query_ptr_c = xcb_query_pointer_unchecked(xc->conn,
			xc->screen.xscreen->root);
	query_ptr_r = xcb_query_pointer_reply(xc->conn, query_ptr_c,
			NULL);

	if(query_ptr_r) {
		uint16_t x, y;
		x = query_ptr_r->win_x;
		y = query_ptr_r->win_y;
		free(query_ptr_r);
		if (scan_xinerama_screens(xc, x, y))
			scan_randr_screens(xc, x, y);
	}
	return 0;
}

