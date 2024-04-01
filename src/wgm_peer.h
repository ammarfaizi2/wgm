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
	char		bind_ip[INET6_ADDRSTRLEN];
};

struct wgm_peer_arg {
	char		ifname[IFNAMSIZ];
	char		public_key[128];
	char		*allowed_ips;	/* comma separated list */
	char		bind_ip[INET6_ADDRSTRLEN];
};

void wgm_peer_dump(const struct wgm_peer *peer);
int wgm_peer_add(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_update(int argc, char *argv[], struct wgm_ctx *ctx);
json_object *wgm_peer_array_to_json(const struct wgm_peer *peers, uint16_t nr_peers);
void wgm_peer_free(struct wgm_peer *peer);
int wgm_peer_array_from_json(struct wgm_peer **peers, uint16_t *nr_peers, json_object *jobj);

#endif /* #ifndef WGM__WG_PEER_H */
