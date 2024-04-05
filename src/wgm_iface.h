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
int wgm_iface_load(struct wgm_iface *iface, struct wgm_ctx *ctx, const char *devname);
int wgm_iface_save(const struct wgm_iface *iface, struct wgm_ctx *ctx);

int wgm_iface_add_peer(struct wgm_iface *iface, const struct wgm_peer *peer);
int wgm_iface_del_peer(struct wgm_iface *iface, size_t idx);
int wgm_iface_del_peer_by_pubkey(struct wgm_iface *iface, const char *pubkey);
int wgm_iface_get_peer_by_pubkey(const struct wgm_iface *iface, const char *pubkey, struct wgm_peer **peer);

void wgm_iface_free(struct wgm_iface *iface);
void wgm_iface_dump_json(const struct wgm_iface *iface);

#endif /* #ifndef WGM__WG_IFACE_H */
