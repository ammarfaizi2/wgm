# SPDX-License-Identifier: GPL-2.0-only

SRC_DIR   = ./src
CC        = gcc
CXX       = g++
INCLUDES  = -I./src/third_party/json/include -I./src
DEPFLAGS  = -MT "$@" -MMD -MP -MF "$(@:%.o=%.d)"
CFLAGS    = $(INCLUDES) -Wall -Wextra -Os -ggdb3 -D_GNU_SOURCE $(DEPFLAGS)
CXXFLAGS  = $(INCLUDES) -Wall -Wextra -Os -ggdb3 -D_GNU_SOURCE $(DEPFLAGS)
LDFLAGS   = -Os -ggdb3
LIBS      = -lpthread


WGMD_SOURCES = \
	$(SRC_DIR)/wgm/wgmd.cpp \
	$(SRC_DIR)/wgm/wgmd_ctx.cpp \
	$(SRC_DIR)/wgm/helpers.cpp \
	$(SRC_DIR)/wgm/helpers/file_handle.cpp

WGMD_HEADERS  = $(WGMD_SOURCES:.cpp=.hpp)
WGMD_DEPFILES = $(WGMD_SOURCES:.cpp=.d)
WGMD_TARGET   = wgmd


all: $(WGMD_TARGET)

-include $(WGMD_DEPFILES)

$(WGMD_TARGET): $(WGMD_SOURCES:.cpp=.o)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rfv $(WGMD_TARGET) $(WGMD_SOURCES:.cpp=.o) $(WGMD_DEPFILES)
