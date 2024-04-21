// SPDX-License-Identifier: GPL-2.0-only

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "wgm_cmd_iface.h"

void wgm_cmd_iface_show_usage(const char *app, int show_cmds)
{
}

int wgm_cmd_iface(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}

int wgm_iface_to_json(const struct wgm_iface *iface, json_object **obj)
{
	json_object *ret, *tmp;
	int err = 0;

	ret = json_object_new_object();
	if (!ret)
		return -ENOMEM;

	err |= json_object_object_add(ret, "ifname", json_object_new_string(iface->ifname));
	err |= json_object_object_add(ret, "private_key", json_object_new_string(iface->private_key));
	err |= json_object_object_add(ret, "listen_port", json_object_new_int(iface->listen_port));
	if (err)
		goto out_err;

	err = wgm_array_str_to_json(&iface->addresses, &tmp);
	if (err)
		goto out_err;

	err = json_object_object_add(ret, "addresses", tmp);
	if (err)
		goto out_err;

	err = wgm_array_peer_to_json(&iface->peers, &tmp);
	if (err)
		goto out_err;

	err = json_object_object_add(ret, "peers", tmp);
	if (err)
		goto out_err;

	*obj = ret;
	return 0;

out_err:
	json_object_put(ret);
	return -ENOMEM;
}

int wgm_iface_from_json(struct wgm_iface *iface, const json_object *obj)
{
	return 0;
}

int wgm_iface_copy(struct wgm_iface *dst, const struct wgm_iface *src)
{
	struct wgm_iface tmp;

	tmp = *src;

	if (wgm_array_str_copy(&tmp.addresses, &src->addresses))
		return -ENOMEM;

	if (wgm_array_peer_copy(&tmp.peers, &src->peers)) {
		wgm_array_str_free(&tmp.addresses);
		return -ENOMEM;
	}

	*dst = tmp;
	return 0;
}

void wgm_iface_move(struct wgm_iface *dst, struct wgm_iface *src)
{
	*dst = *src;
	wgm_array_str_move(&dst->addresses, &src->addresses);
	wgm_array_peer_move(&dst->peers, &src->peers);
	wgm_iface_free(src);
}

void wgm_iface_free(struct wgm_iface *iface)
{
	wgm_array_str_free(&iface->addresses);
	wgm_array_peer_free(&iface->peers);
	memset(iface, 0, sizeof(*iface));
}
