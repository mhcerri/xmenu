#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include "draw.h"
#include "util.h"
#include "screen.h"
#include "complete.h"

/*
 * TODO: args
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

/* configs */
int border = 2;
const char *prompt = "> ";
const char *complete_cmd = NULL;
const char *font_name = "-*-fixed-medium-r-normal-*-13-*-*-*-*-*-iso8859-*";
const char *normal_bg_color = "#000000";
const char *normal_fg_color = "#ffffff";
const char *prompt_fg_color = "#285577";
const char *hlight_bg_color = "#285577";
const char *hlight_fg_color = "#ffffff";

/* state vars */
int loop = 1;
struct x_context xc;
struct geom geom;
xcb_window_t window;
int shift_down = 0;
int numlock_down = 0;
int cursor = 0;
int highlight = 5;
char buffer[64*1024] = "";
struct list *list = NULL;


int init_x()
{
	uint32_t mask, values[3];

	/* Initialize X */
	if (init_x_context(&xc))
		return 1;
	if (load_font(&xc, font_name))
		return 1;

	/* Define geom */
	if (get_screen_geom(&xc, &geom))
		return 1;
	geom.h = xc.font.height + 2 * border;

	/* Create the window */
	mask = XCB_CW_BACK_PIXEL |
		XCB_CW_OVERRIDE_REDIRECT |
		XCB_CW_EVENT_MASK;
	values[0] = xc.screen->white_pixel;
	values[1] = 1;
	values[2] = XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_KEY_PRESS |
		XCB_EVENT_MASK_KEY_RELEASE;
	window = xcb_generate_id(xc.conn);
	xcb_create_window(xc.conn,                     /* Connection    */
			get_depth(&xc),                /* Depth         */
			window,                        /* Window ID     */
			xc.screen->root,               /* Parent window */
			geom.x, geom.y,                /* X, Y          */
			geom.w, geom.h,                /* Width, Height */
			0,                             /* Border width  */
			XCB_WINDOW_CLASS_INPUT_OUTPUT, /* Class (?)     */
			xc.screen->root_visual,        /* Visual (?)    */
			mask, values);                 /* mask & values */

	/* Configure the window */
	mask = XCB_CONFIG_WINDOW_STACK_MODE;
	values[0] = XCB_STACK_MODE_ABOVE;
	xcb_configure_window(xc.conn, window, mask, values);

	/* Setup pixmap */
	resize_canvas(&xc, geom.w, geom.h);

	/* Show window */
	xcb_map_window(xc.conn, window);
	xcb_flush(xc.conn);

	return 0;
}

int grab_keyboard()
{
	long tries = 1000;
	for (; tries; tries--) {
		xcb_grab_keyboard_cookie_t grabc = xcb_grab_keyboard(
				xc.conn, 1, xc.screen->root,
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
	return (tries <= 0);
}



void handle_expose(xcb_expose_event_t *e)
{
	uint16_t x = border;
	uint16_t y = border - 2; // TODO magic "2" ?
	uint16_t cursor_x;
	size_t len = strlen(buffer);

	/* clean */
	set_background(&xc, get_color(&xc, normal_bg_color));
	set_foreground(&xc, get_color(&xc, normal_bg_color));

	// TODO move to draw.c
	xcb_rectangle_t r = { 0, 0, geom.w, geom.h};
	xcb_poly_fill_rectangle(xc.conn, xc.pm, xc.gc, 1, &r);

	/* draw prompt */
	set_foreground(&xc, get_color(&xc, prompt_fg_color));
	x += draw_text8_gw(&xc, prompt, x, y);
	cursor_x = x;

	/* draw buffer */
	set_foreground(&xc, get_color(&xc, normal_fg_color));
	x += draw_ntext8_gw(&xc, buffer, (len < highlight) ? len : highlight,
			x, y);
	if (highlight < strlen(buffer)) {
		set_background(&xc, get_color(&xc, hlight_bg_color));
		set_foreground(&xc, get_color(&xc, hlight_fg_color));
		draw_text8(&xc, buffer + highlight, x, y);
	}

	/* draw cursor */
	cursor_x += get_ntext8_width(&xc, buffer, cursor);
	if (cursor <= highlight) {
		set_foreground(&xc, get_color(&xc, normal_fg_color));
	} else {
		set_foreground(&xc, get_color(&xc, hlight_fg_color));
	}
	draw_line(&xc, cursor_x, y + 2, cursor_x, y + xc.font.height);

	/* copy pixmap */
	xcb_copy_area(xc.conn, xc.pm, window, xc.gc, 0, 0, 0, 0,
			geom.w, geom.h);
	xcb_flush(xc.conn);
}

int special_keypress(xcb_key_press_event_t *e)
{
	size_t len;
	int i, col = 0;
	xcb_keysym_t ksym;

	/* check shift-independent keys */
	ksym = xcb_key_symbols_get_keysym(xc.key_symbols, e->detail, col);
	switch (ksym) {
	case XK_Return:
		printf("%s\n", buffer);
	case XK_Escape:
		loop = 0;
		break;
	case XK_Shift_L:
	case XK_Shift_R:
		shift_down = 1;
		break;
	case XK_Num_Lock: /* TODO */
		numlock_down = 1;
		break;
	case XK_Left:
		if (cursor)
			cursor--;
		highlight = -1;
		break;
	case XK_Right:
		if (cursor < strlen(buffer)) {
			cursor++;
		}
		highlight = -1;
		break;
	case XK_Home:
		cursor = 0;
		highlight = -1;
		break;
	case XK_End:
		cursor = strlen(buffer);
		highlight = -1;
		break;
	case XK_BackSpace:
		if (!cursor)
			break;
		highlight = -1;
		cursor--;
	case XK_Delete:
		for (i = cursor; buffer[i]; i++) {
			buffer[i] = buffer[i + 1];
		}
		highlight = -1;
		break;
	case XK_Tab:
		if (complete_cmd == NULL)
			break;
		len = strlen(buffer);
		if (list == NULL || highlight == -1) {
			free_list(list);
			list = complete(complete_cmd, buffer);
		}
		if (next(list)) {
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
		break;
	default:
		return 1;
	}
	return 0;
}

void handle_keypress(xcb_key_press_event_t *e)
{
	if (!special_keypress(e))
		return;

	/* shift-dependent keys */
	/* TODO check col=2 and col=3 */
	int col = shift_down;
	xcb_keysym_t ksym = xcb_key_symbols_get_keysym(xc.key_symbols,
			e->detail, col);

	/* isgraph can't handle value greater than ushort max */
	if (ksym < 128 && (isgraph(ksym) || isspace(ksym))) {
		int i;
		size_t len = strlen(buffer);
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

void handle_keyrelease(xcb_key_release_event_t *e)
{
	int col = 0;
	xcb_keysym_t ksym = xcb_key_symbols_get_keysym(
			xc.key_symbols, e->detail, col);
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

int main(int argc, char **argv)
{
	int rc = 0;

	/* TODO Parse command line options */
	int i;
	for (i = 1; i < argc; i++) {
		if (strcmp("-c", argv[i]) == 0) {
			if ((i + 1) >= argc) {
				fprintf(stderr, "\"%s\" requires an "
						"argument.\n", argv[i]);
				goto error;
			}
			complete_cmd = argv[++i];
		}
	}

	/* Initialize X context */
	memset(&xc, 0, sizeof(xc));
	if (init_x())
		goto error;
	handle_expose(NULL);
	grab_keyboard();

	/* Event loop */
	xcb_generic_event_t *event;
	while (loop && (event = xcb_wait_for_event(xc.conn))) {
		switch (event->response_type & ~0x80) {
		/* TODO improve how a handler notify to quit */
		case XCB_EXPOSE:
			handle_expose((xcb_expose_event_t*) event);
			break;
		case XCB_KEY_PRESS:
			handle_keypress((xcb_key_press_event_t*) event);
			/* redraw to reflect changes */
			handle_expose(NULL);
			break;
		case XCB_KEY_RELEASE:
			handle_keyrelease((xcb_key_release_event_t*) event);
			break;
		}
		free(event);
	}

exit:
	free_list(list);
	xcb_ungrab_keyboard(xc.conn, XCB_TIME_CURRENT_TIME);
	free_x_context(&xc);
	return rc;
error:
	rc = 1;
	goto exit;
}

