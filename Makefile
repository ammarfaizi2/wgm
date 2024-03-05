
CFLAGS = -Os -Wall -Wextra -ggdb3
LDLIBS = -ljson-c

all: wgm
wgm: wgm.c
clean:
	rm -f wgm

.PHONY: all clean
