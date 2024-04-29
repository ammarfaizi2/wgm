// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_CMD_PEER_H
#define WGM__WG_CMD_PEER_H

#include "wgm.h"

#include <arpa/inet.h>
#include <linux/if.h>

struct wgm_peer {
	char			public_key[128];
	struct wgm_array_str	addresses;
	char			bind_gateway[INET6_ADDRSTRLEN];
	char			bind_ip[INET6_ADDRSTRLEN];
	char			bind_dev[IFNAMSIZ];
};

struct wgm_array_peer {
	struct wgm_peer	*arr;
	size_t		len;
};

void wgm_cmd_peer_show_usage(const char *app, int show_cmds);
int wgm_cmd_peer(int argc, char *argv[], struct wgm_ctx *ctx);

int wgm_peer_to_json(const struct wgm_peer *peer, json_object **ret);
int wgm_peer_from_json(struct wgm_peer *peer, const json_object *obj);

int wgm_peer_copy(struct wgm_peer *dst, const struct wgm_peer *src);
void wgm_peer_move(struct wgm_peer *dst, struct wgm_peer *src);
void wgm_peer_free(struct wgm_peer *peer);

int wgm_array_peer_add(struct wgm_array_peer *arr, const struct wgm_peer *peer);
int wgm_array_peer_del(struct wgm_array_peer *arr, size_t idx);
int wgm_array_peer_find(struct wgm_array_peer *arr, const char *pub_key, size_t *idx);
int wgm_array_peer_add_unique(struct wgm_array_peer *arr, const struct wgm_peer *peer);
int wgm_array_peer_add_mv(struct wgm_array_peer *arr, struct wgm_peer *peer);
int wgm_array_peer_add_mv_unique(struct wgm_array_peer *arr, struct wgm_peer *peer);
int wgm_array_peer_copy(struct wgm_array_peer *dst, const struct wgm_array_peer *src);
void wgm_array_peer_move(struct wgm_array_peer *dst, struct wgm_array_peer *src);
void wgm_array_peer_free(struct wgm_array_peer *peer_array);

int wgm_array_peer_to_json(const struct wgm_array_peer *arr, json_object **obj);
int wgm_array_peer_from_json(struct wgm_array_peer *arr, const json_object *obj);


static inline int wgm_json_obj_set_peer_array(json_object *obj, const char *key,
					      const struct wgm_array_peer *arr)
{
	json_object *val;
	int ret;

	ret = wgm_array_peer_to_json(arr, &val);
	if (ret)
		return ret;

	ret = json_object_object_add(obj, key, val);
	if (ret)
		json_object_put(val);

	return ret;
}

#endif /* #ifndef WGM__WG_CMD_PEER_H */
