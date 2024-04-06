// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_WGM_H
#define WGM__WG_WGM_H

#include "helpers.h"
#include <json-c/json.h>

struct wgm_ctx {
	char	*data_dir;
};

#define WGM_JSON_FLAGS (JSON_C_TO_STRING_NOSLASHESCAPE | JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY)

void show_usage_iface(const char *app, bool show_cmds);
void show_usage_peer(const char *app, bool show_cmds);

#endif /* #ifndef WGM__WG_WGM_H */
