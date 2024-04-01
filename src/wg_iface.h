// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_IFACE_H
#define WGM__WG_IFACE_H

#include "wgm.h"
#include "helpers.h"

#include <stdint.h>
#include <linux/if.h>

struct wgm_iface {
	char		ifname[IFNAMSIZ];
	uint16_t	listen_port;
	char		private_key[128];

	char		**addresses;
	uint16_t	nr_addresses;

	char		**allowed_ips;
	uint16_t	nr_allowed_ips;

	struct wgm_peer	*peers;
	uint16_t	nr_peers;
};

struct wgm_iface_arg {
	char		ifname[IFNAMSIZ];
	uint16_t	listen_port;
	char		private_key[128];
};

int wgm_iface_add(int argc, char *argv[], struct wgm_ctx *ctx);

#endif /* #ifndef WGM__WG_IFACE_H */
