// SPDX-License-Identifier: GPL-2.0-only

#include "wgm_peer.h"

int wgm_peer_cmd_add(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}

int wgm_peer_cmd_del(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}

int wgm_peer_cmd_show(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}

int wgm_peer_cmd_update(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}

int wgm_peer_copy(struct wgm_peer *dst, const struct wgm_peer *src)
{
	memcpy(dst->public_key, src->public_key, sizeof(dst->public_key));
	memcpy(dst->bind_ip, src->bind_ip, sizeof(dst->bind_ip));
	return wgm_str_array_copy(&dst->allowed_ips, &src->allowed_ips);
}

void wgm_peer_free(struct wgm_peer *peer)
{
	free(peer->allowed_ips.arr);
	memset(peer, 0, sizeof(*peer));
}
