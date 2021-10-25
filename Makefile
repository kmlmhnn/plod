CFLAGS=-Wall -Wextra -ofast `pkg-config --cflags libnotify glib-2.0 libcanberra`
LDFLAGS=-lncurses `pkg-config --libs libnotify glib-2.0 libcanberra`

all: plod

plod: plod.c
	gcc plod.c $(CFLAGS) $(LDFLAGS) -o plod

clean:
	rm plod
