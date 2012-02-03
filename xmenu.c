#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include "common.h"
#include "draw.h"
#include "util.h"
#include "screen.h"
#include "complete.h"
#include "extern_complete.h"

/*
 * TODO: completion
 * TODO: loop to get the keyboard
 * TODO: add awesome copyright in screen.c
 * TODO: caps lock and num lock
 * TODO: option list.
 * TODO: show part completed.
 * TODO: config/args
 * TODO: color
 * TODO: unicode support.
 * TODO: use font descent to compensate y when printing text (?)
 * TODO: clipboard
 * TODO: scroll text when too longer
 * TODO: fix randr
 */

char *prompt = "> ";
int border = 2; // TODO maybe margin?
int shift_down = 0;
int numlock_down = 0;
char buffer[64*1024] = "";
int cursor = 0;
int highlight = 5;
int loop = 1;
struct list *list = NULL;

// TODO add a 2nd step to convert these value to a color value
const char *normal_bg_color;
const char *normal_fg_color;
const char *prompt_fg_color;
const char *hlight_bg_color;
const char *hlight_fg_color;

int parse_opts(struct x_context *xc, int argc, char **argv)
{
	/* TODO stub */
	/* TODO load the default values */
	xc->config.font = "-*-fixed-medium-r-normal-*-13-*-*-*-*-*-iso8859-*";
	//xc->config.font = "-*-fixed-medium-r-normal-*-20-*-*-*-*-*-iso8859-*";
	//xc->config.font = "-*-lucida-medium-r-normal-*-20-*-*-*-*-*-iso8859-*";
	normal_bg_color = "#000000";
	normal_fg_color = "#ffffff";
	prompt_fg_color = "#285577";
	hlight_bg_color = "#285577";
	hlight_fg_color = "#ffffff";
	return 0;
}

void show_help(FILE *out)
{
	fprintf(out, "TODO: help\n");
}

int init_window(struct x_context *xc)
{
	uint32_t mask, values[3];

	xc->w = xc->screen.w;
	xc->h = xc->font.height + 2 * border;
	int d = get_depth(xc); /* this is important to pixmap... */

	/* Create the window */
	mask = XCB_CW_BACK_PIXEL |
		XCB_CW_OVERRIDE_REDIRECT |
		XCB_CW_EVENT_MASK;
	values[0] = xc->screen.xscreen->white_pixel;
	values[1] = 1;
	values[2] = XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_KEY_PRESS |
		XCB_EVENT_MASK_KEY_RELEASE;
	xc->win = xcb_generate_id(xc->conn);
	xcb_void_cookie_t win_c = xcb_create_window(
			xc->conn,                        /* Connection    */
			d,                               /* Depth         */
			xc->win,                         /* Window ID     */
			xc->screen.xscreen->root,        /* Parent window */
			xc->screen.x, xc->screen.y,      /* X, Y          */
			xc->w, xc->h,                    /* Width, Height */
			0,                               /* Border width  */
			XCB_WINDOW_CLASS_INPUT_OUTPUT,   /* Class (?)     */
			xc->screen.xscreen->root_visual, /* Visual (?)    */
			mask, values);                   /* mask & values */

	/* Configure the window */
	mask = XCB_CONFIG_WINDOW_STACK_MODE;
	values[0] = XCB_STACK_MODE_ABOVE;
	xcb_configure_window(xc->conn, xc->win, mask, values);

	/* Setup pixmap */
	xc->pm = xcb_generate_id(xc->conn);
	xcb_create_pixmap(xc->conn,                      /* Connection    */
			d,                               /* Depth         */
			xc->pm,                          /* Pixmap id     */
			xc->screen.xscreen->root,        /* Drawable      */
			xc->w,                           /* Width         */
			xc->h);                          /* Height        */

	/* Show window */
	xcb_map_window(xc->conn, xc->win);
	xcb_flush(xc->conn);

	return 0;
}
int init_x_context(struct x_context* xc)
{
	uint32_t mask, values[16];
	/* Connect to X server. */
	xc->conn = xcb_connect(NULL, NULL); /* TODO use args */
	if (xc->conn == NULL || xcb_connection_has_error(xc->conn)) {
		perror("Failed to connect to X server.");
		return 1;
	}

	if (scan_screens(xc)) {
		perror("Failed to get information about the screens.");
		return 1;
	}

	/* Setup font */
	xc->key_symbols = xcb_key_symbols_alloc(xc->conn);
	xc->font.xcb_font = xcb_generate_id(xc->conn);
	xcb_open_font(xc->conn, xc->font.xcb_font,
			strlen(xc->config.font), xc->config.font);

	/* Setup Graphic Context */
	xc->gc = xcb_generate_id(xc->conn);
	mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
	values[0] = xc->screen.xscreen->black_pixel;
	values[1] = xc->screen.xscreen->white_pixel;
	values[2] = xc->font.xcb_font;
	xcb_create_gc(xc->conn, xc->gc, xc->screen.xscreen->root, mask, values);

	/* Get font height */
	xcb_query_font_cookie_t font_cookie = xcb_query_font(
			xc->conn, xc->gc);
	xcb_query_font_reply_t *font_info = xcb_query_font_reply(
			xc->conn, font_cookie, NULL);
	if (font_info == NULL) {
		perror("Failed to get font information.");
		return 1;
	}
	xc->font.height = font_info->max_bounds.ascent
		+ font_info->max_bounds.descent;
	//printf("%d %d\n", font_info->max_bounds.ascent,
	//		font_info->max_bounds.descent);
	free(font_info);

	if(init_window(xc))
		return 1;

	return 0;
}

void handle_expose(struct x_context *xc, xcb_expose_event_t *e)
{
	uint16_t x = border;
	uint16_t y = border - 2; // TODO magic "2" ?
	uint16_t cursor_x;
	size_t len = strlen(buffer);

	/* clean */
	set_background(xc, get_color(xc, normal_bg_color));
	set_foreground(xc, get_color(xc, normal_bg_color));
	xcb_rectangle_t r = { 0, 0, xc->w, xc->h};
	xcb_poly_fill_rectangle(xc->conn, xc->pm, xc->gc, 1, &r);

	/* draw prompt */
	set_foreground(xc, get_color(xc, prompt_fg_color));
	x += draw_text8_gw(xc, prompt, x, y);
	cursor_x = x;

	/* draw buffer */
	set_foreground(xc, get_color(xc, normal_fg_color));
	x += draw_ntext8_gw(xc, buffer, (len < highlight) ? len : highlight, x, y);
	if (highlight < strlen(buffer)) {
		set_background(xc, get_color(xc, hlight_bg_color));
		set_foreground(xc, get_color(xc, hlight_fg_color));
		draw_text8(xc, buffer + highlight, x, y);
	}

	/* draw cursor */
	cursor_x += get_ntext8_width(xc, buffer, cursor);
	if (cursor <= highlight) {
		set_foreground(xc, get_color(xc, normal_fg_color));
	} else {
		set_foreground(xc, get_color(xc, hlight_fg_color));
	}
	draw_line(xc, cursor_x, y + 2, cursor_x, y + xc->font.height);

	/* copy pixmap */
	xcb_copy_area(xc->conn, xc->pm, xc->win, xc->gc, 0, 0, 0, 0,
			xc->w, xc->h);
	xcb_flush(xc->conn);
}

void handle_keypress(struct x_context *xc, xcb_key_press_event_t *e)
{
	size_t len;
	int i, col = 0;
	xcb_keysym_t ksym;

	/* check shift-independent keys */
	ksym = xcb_key_symbols_get_keysym(xc->key_symbols, e->detail, col);
	switch (ksym) {
	case XK_Return:
		printf("%s\n", buffer);
	case XK_Escape:
		loop = 0;
		return;
	case XK_Shift_L:
	case XK_Shift_R:
		shift_down = 1;
		return;
	case XK_Num_Lock: /* TODO */
		numlock_down = 1;
		printf("NL down\n");
		return;
	case XK_Left:
		if (cursor)
			cursor--;
		highlight = -1;
		return;
	case XK_Right:
		if (cursor < strlen(buffer)) {
			cursor++;
		}
		highlight = -1;
		return;
	case XK_Home:
		cursor = 0;
		highlight = -1;
		return;
	case XK_End:
		cursor = strlen(buffer);
		highlight = -1;
		return;
	case XK_BackSpace:
		if (!cursor)
			return;
		highlight = -1;
		cursor--;
	case XK_Delete:
		for (i = cursor; buffer[i]; i++) {
			buffer[i] = buffer[i + 1];
		}
		highlight = -1;
		return;
	case XK_Tab:
		len = strlen(buffer);
		if (list == NULL || highlight == -1) {
			free_list(list);
			list = extern_complete("complete.sh", buffer);
		}
		if (next(list)) {
			//strcpy(buffer, list->cur->name);
			if (highlight == -1)
				highlight = len;
			char *it;
			for (it = buffer; *it; it++)
				;
			for(; it != buffer; it--) {
				if (isspace(*it)) {
					it++;
					break;
				}
			}
			strcpy(it, list->cur->name);
			cursor = strlen(buffer);;
		}

		return; 
	}

	/* shift-dependent keys */
	/* TODO check col=2 and col=3 */
	col = shift_down;
	ksym = xcb_key_symbols_get_keysym(xc->key_symbols, e->detail, col);
	/* isgraph can't handle value greater than ushort max */
	if (ksym < 128 && (isgraph(ksym) || isspace(ksym))) {
		len = strlen(buffer);
		if ((len + 1) < sizeof(buffer)) {
			for (i = len; i >= cursor; i--) {
				buffer[i + 1] = buffer[i];
			}
			buffer[cursor] = ksym;
			cursor++;
			highlight = -1;
		}
	}
}

void handle_keyrelease(struct x_context *xc, xcb_key_release_event_t *e)
{
	int col = 0;
	xcb_keysym_t ksym = xcb_key_symbols_get_keysym(
			xc->key_symbols, e->detail, col);
	switch(ksym) {
	case XK_Shift_L:
	case XK_Shift_R:
		shift_down = 0;
		return;
	case XK_Num_Lock:
		numlock_down = 0;
		printf("NL up\n");
		return;
	}
}

void cleanup_x_context(struct x_context *xc)
{
	if (xc->key_symbols)
		xcb_key_symbols_free(xc->key_symbols);
	if (xc->pm)
		xcb_free_pixmap(xc->conn, xc->pm);
	if (xc->gc)
		xcb_free_gc(xc->conn, xc->gc);
	if (xc->conn) {
		xcb_disconnect(xc->conn);
		xc->conn = NULL;
	}
}

int main(int argc, char **argv)
{
	int rc = 0;
	struct x_context xc;
	memset(&xc, 0, sizeof(xc));

	/* Parse command line options */
	if (parse_opts(&xc, argc, argv))
		goto error;
	if (xc.config.help) {
		show_help(stdout);
		goto exit;
	}

	if (init_x_context(&xc)) {
		goto error;
	}

	/* Grab the keyboard */
	/* TODO add a limit */
	handle_expose(&xc, NULL);
	while (1) {
		xcb_grab_keyboard_cookie_t grabc = xcb_grab_keyboard(
				xc.conn, 1, xc.screen.xscreen->root,
				XCB_TIME_CURRENT_TIME, XCB_GRAB_MODE_ASYNC,
				XCB_GRAB_MODE_ASYNC);
		xcb_grab_keyboard_reply_t *grabr = xcb_grab_keyboard_reply(
				xc.conn, grabc, NULL);
		if (grabr) {
			int status = grabr->status;
			free(grabr);
			if (!status) {
				break;
			}
		}
		usleep(1000);
	}
	xcb_flush(xc.conn);

	/* Event loop */
	xcb_generic_event_t *event;
	while (loop && (event = xcb_wait_for_event(xc.conn))) {
		switch (event->response_type & ~0x80) {
		/* TODO improve how a handler notify to quit */
		case XCB_EXPOSE:
			handle_expose(&xc, (xcb_expose_event_t*) event);
			break;
		case XCB_KEY_PRESS:
			handle_keypress(&xc, (xcb_key_press_event_t*) event);
			/* redraw to reflect changes */
			handle_expose(&xc, NULL);
			break;
		case XCB_KEY_RELEASE:
			handle_keyrelease(&xc,
					(xcb_key_release_event_t*) event);
			break;
		}
		free(event);
	}

exit:
	free_list(list);
	xcb_ungrab_keyboard(xc.conn, XCB_TIME_CURRENT_TIME);
	cleanup_x_context(&xc);
	return rc;
error:
	rc = 1;
	goto exit;
}

