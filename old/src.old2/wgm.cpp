// SPDX-License-Identifier: GPL-2.0-only

#include "wgm.h"
#include "wgm_cmd_iface.h"
#include "wgm_cmd_peer.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static void show_usage(const char *app)
{
	printf("Usage: %s [iface|peer] <options>\n\n", app);
	printf("See '%s iface --help' or '%s peer --help' for more information.\n", app, app);
}

static char *dup_getenv(const char *key, const char *def)
{
	const char *val = getenv(key);
	if (!val)
		val = def;

	return strdup(val);
}

static void wgm_ctx_free(struct wgm_ctx *ctx)
{
	free(ctx->data_dir);
	free(ctx->wg_quick_path);
	free(ctx->wg_conf_path);
	memset(ctx, 0, sizeof(*ctx));
	wgm_err_elog_flush();
}

static int wgm_ctx_init(struct wgm_ctx *ctx)
{
	int ret;

	memset(ctx, 0, sizeof(*ctx));
	ctx->data_dir = dup_getenv("WGM_DATA_DIR", "./wgm_data");
	ctx->wg_quick_path = dup_getenv("WGM_WG_QUICK_PATH", "/usr/bin/wg-quick");
	ctx->wg_conf_path = dup_getenv("WGM_WG_CONF_PATH", "/etc/wireguard");

	if (!ctx->data_dir || !ctx->wg_quick_path || !ctx->wg_conf_path) {
		ret = -ENOMEM;
		goto out_err;
	}

	ret = wgm_mkdir_recursive(ctx->data_dir, 0700);
	if (ret)
		goto out_err;

	return 0;

out_err:
	wgm_ctx_free(ctx);
	return ret;
}

static int wgm_ctx_run(int argc, char *argv[], struct wgm_ctx *ctx)
{
	if (!strcmp(argv[1], "iface"))
		return wgm_cmd_iface(argc, argv, ctx);
	else if (!strcmp(argv[1], "peer"))
		return wgm_cmd_peer(argc, argv, ctx);

	return -EINVAL;
}

int main(int argc, char *argv[])
{
	int ret = 1;

	if (argc >= 2) {
		struct wgm_ctx ctx;

		ret = wgm_ctx_init(&ctx);
		if (!ret)
			ret = wgm_ctx_run(argc, argv, &ctx);
		wgm_ctx_free(&ctx);
	} else {
		show_usage(argv[0]);
	}

	return abs(ret);
}
