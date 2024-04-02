// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_PEER_H
#define WGM__WG_PEER_H

#include "wgm.h"
#include "helpers.h"

#include <stdint.h>

struct wgm_peer {
	char		public_key[128];
	struct wgm_str_array allowed_ips;
	char		bind_ip[INET6_ADDRSTRLEN];
};

struct wgm_peer_arg {
	char		ifname[IFNAMSIZ];
	char		public_key[128];
	char		*allowed_ips;	/* comma separated list */
	char		bind_ip[INET6_ADDRSTRLEN];
	bool		force;
};

int wgm_peer_cmd_add(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_cmd_del(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_cmd_show(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_cmd_update(int argc, char *argv[], struct wgm_ctx *ctx);

void wgm_peer_free(struct wgm_peer *peer);
void wgm_peer_dump(const struct wgm_peer *peer);

int wgm_peer_to_json(const struct wgm_peer *peer, json_object **obj);
int wgm_json_to_peer(struct wgm_peer *peer, const json_object *obj);

#endif /* #ifndef WGM__WG_PEER_H */
