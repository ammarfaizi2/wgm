// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_CMD_PEER_H
#define WGM__WG_CMD_PEER_H

#include "wgm.h"

void wgm_cmd_peer_show_usage(const char *app, int show_cmds);
int wgm_cmd_peer(int argc, char *argv[], struct wgm_ctx *ctx);

#endif /* #ifndef WGM__WG_CMD_PEER_H */
