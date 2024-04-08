// SPDX-License-Identifier: GPL-2.0-only

#include "wgm_conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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

static int wgm_conf_write_iptables(FILE *h, const struct wgm_iface *iface)
{
	size_t i, j;

	fprintf(h, "\n");

	fprintf(h, "PostUp   = iptables -t nat -F wgm_%s || true >> /dev/null 2>&1\n", iface->ifname);
	fprintf(h, "PostUp   = iptables -t nat -N wgm_%s || true >> /dev/null 2>&1\n", iface->ifname);
	fprintf(h, "PostUp   = iptables -t nat -I POSTROUTING -j wgm_%s || true >> /dev/null 2>&1\n", iface->ifname);
	fprintf(h, "PostDown = iptables -t nat -D POSTROUTING -j wgm_%s\n", iface->ifname);
	fprintf(h, "PostDown = iptables -t nat -F wgm_%s\n", iface->ifname);
	fprintf(h, "PostDown = iptables -t nat -X wgm_%s\n", iface->ifname);

	fprintf(h, "\n");

	fprintf(h, "PostUp   = iptables -t filter -F wgm_%s || true >> /dev/null 2>&1\n", iface->ifname);
	fprintf(h, "PostUp   = iptables -t filter -N wgm_%s || true >> /dev/null 2>&1\n", iface->ifname);
	fprintf(h, "PostUp   = iptables -t filter -I FORWARD -j wgm_%s || true >> /dev/null 2>&1\n", iface->ifname);
	fprintf(h, "PostDown = iptables -t filter -D POSTROUTING -j wgm_%s\n", iface->ifname);
	fprintf(h, "PostDown = iptables -t filter -F wgm_%s\n", iface->ifname);
	fprintf(h, "PostDown = iptables -t filter -X wgm_%s\n", iface->ifname);

	fprintf(h, "\n");

	for (i = 0; i < iface->peers.nr; i++) {
		const struct wgm_peer *peer = &iface->peers.peers[i];

		if (peer->allowed_ips.nr)
			fprintf(h, "### Start for peer %s\n", peer->public_key);

		for (j = 0; j < peer->allowed_ips.nr; j++) {
			const char *src = peer->allowed_ips.arr[j];

			fprintf(h, "PostUp   = iptables -t filter -A wgm_%s -s %s -j ACCEPT\n", iface->ifname, src);
			if (peer->bind_ip[0])
				fprintf(h, "PostUp   = iptables -t nat -A wgm_%s -s %s -j SNAT --to %s\n", iface->ifname, src, peer->bind_ip);
		}

		/*
		 * Repeat the loop for any client who doesn't have a bind IP.
		 * MASQUERADE must be placed at the end, so the loop needs to
		 * be repeated.
		 */
		for (j = 0; j < peer->allowed_ips.nr; j++) {
			const char *src = peer->allowed_ips.arr[j];

			if (!peer->bind_ip[0])
				fprintf(h, "PostUp   = iptables -t nat -A wgm_%s -s %s -j MASQUERADE\n", iface->ifname, src);
		}

		if (peer->allowed_ips.nr)
			fprintf(h, "### End for peer %s\n", peer->public_key);
	}

	fprintf(h, "\n");
	fprintf(h, "PostUp   = iptables -t nat -A wgm_%s -j RETURN\n", iface->ifname);
	fprintf(h, "PostUp   = iptables -t filter -A wgm_%s -j RETURN\n", iface->ifname);
	return 0;
}

static int wgm_conf_write(FILE *h, const struct wgm_iface *iface)
{
	size_t i, n;
	int ret;

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
	ret = wgm_conf_write_iptables(h, iface);
	if (ret)
		return ret;

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
