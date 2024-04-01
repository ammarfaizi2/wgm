// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_IFACE_H
#define WGM__WG_IFACE_H

#include "wgm.h"
#include "helpers.h"

#include <stdint.h>
#include <linux/if.h>

struct wgm_peer;

struct wgm_peer_array {
	struct wgm_peer	*peers;
	size_t		nr;
};

struct wgm_iface {
	char		ifname[IFNAMSIZ];
	uint16_t	listen_port;
	char		private_key[128];

	struct wgm_str_array addresses;
	struct wgm_str_array allowed_ips;
	struct wgm_peer_array peers;
};

struct wgm_iface_arg {
	bool		force;
	char		ifname[IFNAMSIZ];
	uint16_t	listen_port;
	char		private_key[128];	
};

int wgm_iface_add(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_iface_update(int argc, char *argv[], struct wgm_ctx *ctx);

int wgm_iface_load(struct wgm_ctx *ctx, struct wgm_iface *iface, const char *ifname);
int wgm_iface_save(struct wgm_ctx *ctx, const struct wgm_iface *iface);

void wgm_peer_array_dump(const struct wgm_peer_array *peers);
void wgm_iface_dump(const struct wgm_iface *iface);
void wgm_iface_free(struct wgm_iface *iface);

#endif /* #ifndef WGM__WG_IFACE_H */
