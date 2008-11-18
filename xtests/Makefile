CC = gcc

CFLAGS = -O2 -Wall -m486 -fomit-frame-pointer -I/usr/include/X11
XTLIBS = -lm -lXaw -lXt -lXext -lX11
X11LIBS = -lX11
XMLIBS = -lXm -lXt -lX11

#CFLAGS = +Aa +O2 -I/usr/include/Motif1.1
#XMLIBS = -L/usr/lib/Motif1.2 -L/usr/lib/X11R5

all: x11mouse xmmenu xmserv

x11mouse: x11mouse.o
	$(CC) -o $@ x11mouse.o $(X11LIBS)

xmmenu: xmmenu.o
	$(CC) -o $@ xmmenu.o $(XMLIBS)

xmserv: xmserv.o
	$(CC) -o $@ xmserv.o $(XMLIBS)

