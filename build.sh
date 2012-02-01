#!/bin/bash
gcc -g -o xrun xrun.c draw.c screen.c util.c complete.c -lxcb -lxcb-keysyms -lxcb-randr -lxcb-xinerama
