
CFLAGS = -static -Os -Wall -Wextra -ggdb3 -D_GNU_SOURCE
LDFLAGS = -Os -static
LDLIBS = -ljson-c

HEADER_FILES = src/wg_iface.h src/wg_peer.h src/wgm.h src/helpers.h
SOURCE_FILES = src/wg_iface.c src/wg_peer.c src/wgm.c src/helpers.c
OBJECT_FILES = $(SOURCE_FILES:.c=.o)

all: wgm

wgm: $(OBJECT_FILES)
	$(CC) $(LDFLAGS) -o $@ $(OBJECT_FILES) $(LDLIBS)

%.o: %.c $(HEADER_FILES)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f wgm $(OBJECT_FILES)

.PHONY: all clean
