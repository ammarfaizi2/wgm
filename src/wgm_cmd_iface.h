// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_CMD_IFACE_H
#define WGM__WG_CMD_IFACE_H

#include "wgm.h"
#include "wgm_cmd_peer.h"

#include <linux/if.h>

struct wgm_iface {
	char			ifname[IFNAMSIZ];
	char			private_key[128];
	uint16_t		listen_port;
	uint16_t		mtu;
	struct wgm_array_str	addresses;
	struct wgm_array_peer	peers;
};

struct wgm_iface_hdl {
	struct wgm_ctx		*ctx;
	wgm_file_t		file;
};

struct wgm_array_iface {
	struct wgm_iface	*arr;
	size_t			len;
};

void wgm_cmd_iface_show_usage(const char *app, int show_cmds);
int wgm_cmd_iface(int argc, char *argv[], struct wgm_ctx *ctx);

int wgm_iface_hdl_open(struct wgm_iface_hdl *hdl, const char *dev);
int wgm_iface_hdl_close(struct wgm_iface_hdl *hdl);
int wgm_iface_hdl_load(struct wgm_iface_hdl *hdl, struct wgm_iface *iface);
int wgm_iface_hdl_store(struct wgm_iface_hdl *hdl, struct wgm_iface *iface);

int wgm_iface_to_json(const struct wgm_iface *iface, json_object **ret);
int wgm_iface_from_json(struct wgm_iface *iface, const json_object *obj);

int wgm_iface_copy(struct wgm_iface *dst, const struct wgm_iface *src);
void wgm_iface_move(struct wgm_iface *dst, struct wgm_iface *src);
void wgm_iface_free(struct wgm_iface *iface);

int wgm_array_iface_add(struct wgm_array_iface *arr, const struct wgm_iface *iface);
int wgm_array_iface_del(struct wgm_array_iface *arr, size_t idx);
int wgm_array_iface_find(struct wgm_array_iface *arr, const char *ifname, size_t *idx);
int wgm_array_iface_add_unique(struct wgm_array_iface *arr, const struct wgm_iface *iface);
int wgm_array_iface_copy(struct wgm_array_iface *dst, const struct wgm_array_iface *src);
void wgm_array_iface_free(struct wgm_array_iface *arr);
void wgm_array_iface_move(struct wgm_array_iface *dst, struct wgm_array_iface *src);

#endif /* #ifndef WGM__WG_CMD_IFACE_H */
