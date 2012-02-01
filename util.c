#include "util.h"
#include <string.h>
#include <xcb/xcb.h>
#include "common.h"

xcb_char2b_t* convert_ascii_to_char2b(const char *ascii)
{
	size_t i, len;
	xcb_char2b_t *wstr;
	if (ascii == NULL)
		return NULL;
	len = strlen(ascii);
	wstr = malloc(sizeof(xcb_char2b_t) * (len + 1));
	if (wstr == NULL)
		return NULL;
	for (i = 0; i <= len; i++) {
		wstr[i].byte1 = 0;
		wstr[i].byte2 = ascii[i];
	}
	return wstr;
}

