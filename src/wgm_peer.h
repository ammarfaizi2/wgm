// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_PEER_H
#define WGM__WG_PEER_H

#include "helpers.h"

struct wgm_peer {
	char			public_key[256];
	char			endpoint[128];
	char			bind_ip[16];
	char			bind_dev[IFNAMSIZ];
	struct wgm_str_array	allowed_ips;
};

int wgm_peer_cmd_add(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_cmd_del(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_cmd_show(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_cmd_update(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_cmd_list(int argc, char *argv[], struct wgm_ctx *ctx);
int wgm_peer_copy(struct wgm_peer *dst, const struct wgm_peer *src);
void wgm_peer_move(struct wgm_peer *dst, struct wgm_peer *src);
void wgm_peer_free(struct wgm_peer *peer);

int wgm_peer_to_json(json_object **jobj, const struct wgm_peer *peer);
int wgm_peer_from_json(struct wgm_peer *peer, const json_object *jobj);

#endif /* #ifndef WGM__WG_PEER_H */
