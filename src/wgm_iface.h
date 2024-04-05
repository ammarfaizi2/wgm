// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_IFACE_H
#define WGM__WG_IFACE_H

#include "helpers.h"

struct wgm_peer;

struct wgm_peer_array {
	struct wgm_peer	*peers;
	size_t		nr;
};

struct wgm_iface {
	char			ifname[IFNAMSIZ];
	uint16_t		listen_port;
	uint16_t		mtu;
	char			private_key[128];

	struct wgm_str_array	addresses;
	struct wgm_str_array	allowed_ips;
	struct wgm_peer_array	peers;
};

int wgm_iface_cmd_add(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_iface_cmd_del(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_iface_cmd_show(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_iface_cmd_update(int argc, char *argv[], struct wgm_ctx *ctx);

#endif /* #ifndef WGM__WG_IFACE_H */
