#!/bin/bash
gcc -Wall -g -o xmenu xmenu.c draw.c screen.c util.c complete.c -lxcb -lxcb-keysyms -lxcb-xinerama -DXINERAMA
