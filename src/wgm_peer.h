// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_PEER_H
#define WGM__WG_PEER_H

#include "helpers.h"

int wgm_peer_cmd_add(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_cmd_del(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_cmd_show(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_cmd_update(int argc, char *argv[], struct wgm_ctx *ctx);

#endif /* #ifndef WGM__WG_PEER_H */
