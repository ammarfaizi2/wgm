
CFLAGS = -Os -Wall -Wextra -ggdb3 -D_GNU_SOURCE -Wno-unused-parameter -xc
CXXFLAGS = -Os -Wall -Wextra -ggdb3 -D_GNU_SOURCE -Wno-unused-parameter -xc
LDFLAGS = -Os
LDLIBS = -ljson-c

ifeq ($(SANITIZE),1)
	CFLAGS += -fsanitize=address -fsanitize=undefined
	LDFLAGS += -fsanitize=address -fsanitize=undefined
else
	CFLAGS += -static
	LDFLAGS += -static
endif

SOURCE_FILES = \
	src/wgm.cpp \
	src/wgm_helpers.cpp \
	src/wgm_cmd_iface.cpp \
	src/wgm_cmd_peer.cpp

HEADER_FILES = $(SOURCE_FILES:.cpp=.hpp)
OBJECT_FILES = $(SOURCE_FILES:.cpp=.o)

all: wgm

wgm: $(OBJECT_FILES)
	$(CC) $(LDFLAGS) -o $@ $(OBJECT_FILES) $(LDLIBS)

%.o: %.cpp $(HEADER_FILES)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f wgm $(OBJECT_FILES)

.PHONY: all clean
