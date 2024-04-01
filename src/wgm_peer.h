// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_PEER_H
#define WGM__WG_PEER_H

#include "wgm.h"
#include "helpers.h"

#include <stdint.h>

struct wgm_peer {
	char		public_key[128];
	char		**allowed_ips;
	uint16_t	nr_allowed_ips;
};

struct wgm_peer_arg {
};

int wgm_peer_add(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_update(int argc, char *argv[], struct wgm_ctx *ctx);

#endif /* #ifndef WGM__WG_PEER_H */
