CC := gcc
CFLAGS := -Wall -g
LDFLAGS := -lxcb -lxcb-keysyms -lxcb-xinerama -DXINERAMA
TARGET := xmenu
SOURCES := xmenu.c draw.c screen.c util.c complete.c

$(TARGET):
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

clean:
	rm -rf $(TARGET)

.PHONY: all clean

