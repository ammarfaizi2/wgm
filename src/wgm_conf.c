// SPDX-License-Identifier: GPL-2.0-only

#include "wgm_conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/file.h>

static int get_fwmark_path(char **out, const char *src, const char *ip,
			   struct wgm_ctx *ctx)
{
	char *ret;
	int err;

	err = wgm_asprintf(&ret, "%s/fwmark", ctx->data_dir);
	if (err)
		return err;

	err = mkdir_recursive(ret, 0700);
	if (err) {
		wgm_log_err("Failed to create directory '%s': %s\n", ret, strerror(-err));
		free(ret);
		return err;
	}

	free(ret);
	err = wgm_asprintf(&ret, "%s/fwmark/%s-%s.txt", ctx->data_dir, src, ip);
	if (err)
		return err;

	*out = ret;
	return 0;
}

static int generate_fwmark_auto_increment(unsigned *mark, struct wgm_ctx *ctx)
{
	unsigned tmp;
	char *fpath;
	FILE *fp;
	int ret;

	ret = wgm_asprintf(&fpath, "%s/fwmark.last", ctx->data_dir);
	if (ret)
		return ret;

	fp = fopen(fpath, "rb+");
	if (!fp) {
		fp = fopen(fpath, "wb");
		if (!fp) {
			ret = -errno;
			wgm_log_err("Failed to create fwmark file '%s': %s\n", fpath, strerror(-ret));
			return ret;
		}

		flock(fileno(fp), LOCK_EX);
		fprintf(fp, "%u\n", 37000u);
		fclose(fp);
		*mark = 37000u;
		return 0;
	}

	flock(fileno(fp), LOCK_EX);
	if (fscanf(fp, "%u\n", &tmp) != 1) {
		wgm_log_err("Failed to read fwmark file '%s': Invalid unsigned integer format\n", fpath);
		fclose(fp);
		return -EINVAL;
	}
	rewind(fp);
	fprintf(fp, "%u\n", tmp + 1);
	fclose(fp);
	*mark = tmp;
	return 0;
}

static int get_fwmark(unsigned *mark, const char *src, const char *ip,
		      struct wgm_ctx *ctx)
{
	char *fpath;
	FILE *fp;
	int ret;

	ret = get_fwmark_path(&fpath, src, ip, ctx);
	if (ret)
		return ret;

	fp = fopen(fpath, "rb+");
	if (!fp) {
		unsigned tmp_mark;

		fp = fopen(fpath, "wb");
		if (!fp) {
			ret = -errno;
			wgm_log_err("Failed to create fwmark file '%s': %s\n", fpath, strerror(-ret));
			goto out;
		}

		flock(fileno(fp), LOCK_EX);
		ret = generate_fwmark_auto_increment(&tmp_mark, ctx);
		if (ret)
			goto out;

		ret = 0;
		*mark = tmp_mark;
		fprintf(fp, "%u\n", tmp_mark);
		goto out;
	}

	flock(fileno(fp), LOCK_EX);
	if (fscanf(fp, "%u\n", mark) != 1) {
		wgm_log_err("Failed to read fwmark file '%s': Invalid unsigned integer format\n", fpath);
		ret = -EINVAL;
	}

out:
	fclose(fp);
	if (ret)
		remove(fpath);

	free(fpath);
	return ret;
}

static char *get_conf_path(const struct wgm_iface *iface, struct wgm_ctx *ctx)
{
	char *ret;
	int err;

	err = wgm_asprintf(&ret, "%s/wg_conf", ctx->data_dir);
	if (err < 0)
		return NULL;

	err = mkdir_recursive(ret, 0700);
	if (err) {
		wgm_log_err("Failed to create directory '%s': %s\n", ret, strerror(-err));
		free(ret);
		return NULL;
	}

	free(ret);
	err = wgm_asprintf(&ret, "%s/wg_conf/%s.conf", ctx->data_dir, iface->ifname);
	if (err < 0)
		return NULL;

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

static int wgm_conf_write_iptables(FILE *h, const struct wgm_iface *iface,
				   struct wgm_ctx *ctx)
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
	fprintf(h, "PostDown = iptables -t filter -F wgm_%s\n", iface->ifname);
	fprintf(h, "PostDown = iptables -t filter -X wgm_%s\n", iface->ifname);

	fprintf(h, "\n");

	fprintf(h, "PostUp   = iptables -t mangle -F wgm_%s || true >> /dev/null 2>&1\n", iface->ifname);
	fprintf(h, "PostUp   = iptables -t mangle -N wgm_%s || true >> /dev/null 2>&1\n", iface->ifname);
	fprintf(h, "PostDown = iptables -t mangle -D PREROUTING -j wgm_%s\n", iface->ifname);
	fprintf(h, "PostDown = iptables -t mangle -F wgm_%s\n", iface->ifname);
	fprintf(h, "PostDown = iptables -t mangle -X wgm_%s\n", iface->ifname);

	for (i = 0; i < iface->peers.nr; i++) {
		const struct wgm_peer *peer = &iface->peers.peers[i];

		if (peer->allowed_ips.nr)
			fprintf(h, "\n### Start for peer %s\n", peer->public_key);

		for (j = 0; j < peer->allowed_ips.nr; j++) {
			const char *src = peer->allowed_ips.arr[j];
			unsigned mark;
			int ret;

			fprintf(h, "PostUp   = iptables -t filter -A wgm_%s -s %s -j ACCEPT\n", iface->ifname, src);
			if (peer->bind_ip[0]) {
				ret = get_fwmark(&mark, peer->bind_ip, peer->bind_dev, ctx);
				if (ret)
					return ret;
				fprintf(h, "PostUp   = iptables -t mangle -A wgm_%s -s %s -j MARK --set-mark %u\n", iface->ifname, src, mark);
				fprintf(h, "PostUp   = iptables -t nat -A wgm_%s -s %s -j SNAT --to %s\n", iface->ifname, src, peer->bind_ip);
				fprintf(h, "PostUp   = ip rule add fwmark %u lookup %u\n", mark, mark);
				fprintf(h, "PostDown = ip rule del fwmark %u lookup %u\n", mark, mark);
				fprintf(h, "PostUp   = ip route replace default dev %s table %u\n", peer->bind_dev, mark);
			}
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
	fprintf(h, "PostUp   = iptables -t mangle -A wgm_%s -j RETURN\n", iface->ifname);
	return 0;
}

static int wgm_conf_write(FILE *h, const struct wgm_iface *iface,
			  struct wgm_ctx *ctx)
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
	ret = wgm_conf_write_iptables(h, iface, ctx);
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

	ret = wgm_conf_write(fp, iface, ctx);
	free(path);
	return ret;
}
