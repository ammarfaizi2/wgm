
CFLAGS = -Os -Wall -Wextra -ggdb3 -D_GNU_SOURCE -Wno-unused-parameter
LDFLAGS = -Os
LDLIBS = -ljson-c

ifeq ($(SANITIZE),1)
	CFLAGS += -fsanitize=address -fsanitize=undefined
	LDFLAGS += -fsanitize=address -fsanitize=undefined
else
	CFLAGS += -static
	LDFLAGS += -static
endif

HEADER_FILES = src/wgm_iface.h src/wgm_peer.h src/wgm.h src/helpers.h
SOURCE_FILES = src/wgm_iface.c src/wgm_peer.c src/wgm.c src/helpers.c
OBJECT_FILES = $(SOURCE_FILES:.c=.o)

all: wgm

wgm: $(OBJECT_FILES)
	$(CC) $(LDFLAGS) -o $@ $(OBJECT_FILES) $(LDLIBS)

%.o: %.c $(HEADER_FILES)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f wgm $(OBJECT_FILES)

.PHONY: all clean
