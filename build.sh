#!/bin/bash
gcc -g -o xmenu xmenu.c draw.c screen.c util.c complete.c extern_complete.c -lxcb -lxcb-keysyms -lxcb-randr -lxcb-xinerama
