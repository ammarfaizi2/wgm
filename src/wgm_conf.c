// SPDX-License-Identifier: GPL-2.0-only

#include "wgm_conf.h"

#include <stdio.h>
#include <stdlib.h>

static char *get_conf_path(const struct wgm_iface *iface, struct wgm_ctx *ctx)
{
	size_t len;
	char *ret;

	len = strlen(ctx->data_dir) + strlen(iface->ifname) + 32;
	ret = malloc(len);
	if (!ret)
		return NULL;

	snprintf(ret, len, "%s/%s.conf", ctx->data_dir, iface->ifname);
	return ret;
}

static int fprint_str_arr(FILE *h, const struct wgm_str_array *arr)
{
	size_t i, n = arr->nr;
	int ret = 0;

	for (i = 0; i < n; i++) {
		ret += fprintf(h, "%s", arr->arr[i]);
		if (i < n - 1)
			ret += fprintf(h, ", ");
	}

	return ret;
}

static int wgm_conf_write(FILE *h, const struct wgm_iface *iface)
{
	size_t i, n;

	fprintf(h, "[Interface]\n");
	fprintf(h, "ListenPort = %hu\n", iface->listen_port);
	fprintf(h, "PrivateKey = %s\n", iface->private_key);
	n = iface->addresses.nr;
	fprintf(h, "Address = ");
	for (i = 0; i < n; i++) {
		fprintf(h, "%s", iface->addresses.arr[i]);
		if (i < n - 1)
			fprintf(h, ",");
	}
	fprintf(h, "\n");
	fprintf(h, "AllowedIPs = ");
	fprint_str_arr(h, &iface->allowed_ips);
	fprintf(h, "\n");

	n = iface->peers.nr;
	for (i = 0; i < n; i++) {
		const struct wgm_peer *peer = &iface->peers.peers[i];

		fprintf(h, "\n[Peer]\n");
		fprintf(h, "PublicKey = %s\n", peer->public_key);
		fprintf(h, "AllowedIPs = ");
		fprint_str_arr(h, &peer->allowed_ips);
		fprintf(h, "\n");
		if (peer->endpoint[0])
			fprintf(h, "Endpoint = %s\n", peer->endpoint);
		if (peer->bind_ip[0])
			fprintf(h, "# -- -- BindIP = %s\n", peer->bind_ip);
	}

	return 0;
}

int wgm_conf_save(const struct wgm_iface *iface, struct wgm_ctx *ctx)
{
	char *path = get_conf_path(iface, ctx);
	FILE *fp;
	int ret;

	if (!path)
		return -ENOMEM;

	fp = fopen(path, "wb");
	if (!fp) {
		free(path);
		return -ENOMEM;
	}

	ret = wgm_conf_write(fp, iface);
	free(path);
	return ret;
}
