// SPDX-License-Identifier: GPL-2.0-only

#include "wgm.h"
#include "wgm_iface.h"
#include "wgm_peer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static void show_usage_iface(const char *app)
{
	printf("Usage: %s iface [command] [options]\n", app);
	printf("Commands:\n");
	printf("  add    - Add a new WireGuard interface\n");
	printf("  del    - Delete an existing WireGuard interface\n");
	printf("  show   - Show information about a WireGuard interface\n");
	printf("  update - Update an existing WireGuard interface\n");
	printf("\n");
}

static void show_usage_peer(const char *app)
{
	printf("Usage: %s peer [command] [options]\n", app);
	printf("Commands:\n");
	printf("  add    - Add a new peer to a WireGuard interface\n");
	printf("  del    - Delete an existing peer from a WireGuard interface\n");
	printf("  show   - Show information about a peer in a WireGuard interface\n");
	printf("  update - Update an existing peer in a WireGuard interface\n");
	printf("\n");
}

static int wgm_init_ctx(struct wgm_ctx *ctx)
{
	const char *data_dir = getenv("WGM_DIR");
	int ret;

	if (!data_dir)
		data_dir = "./wgm_data";

	ret = mkdir_recursive(data_dir, 0700);
	if (ret < 0) {
		fprintf(stderr, "Error: failed to create directory: %s: %s\n", data_dir, strerror(-ret));
		return ret;
	}

	ctx->data_dir = strdup(data_dir);
	if (!ctx->data_dir) {
		fprintf(stderr, "Error: failed to allocate memory\n");
		return -ENOMEM;
	}

	return 0;
}

static void wgm_free_ctx(struct wgm_ctx *ctx)
{
	free(ctx->data_dir);
}

static int wgm_run(int argc, char *argv[], struct wgm_ctx *ctx)
{
	if (!strcmp(argv[1], "iface")) {
		if (argc == 2) {
			show_usage_iface(argv[0]);
			return 1;
		}

		if (!strcmp(argv[2], "add"))
			return wgm_iface_cmd_add(argc - 2, argv + 2, ctx);

		if (!strcmp(argv[2], "del"))
			return wgm_iface_cmd_del(argc - 2, argv + 2, ctx);

		if (!strcmp(argv[2], "show"))
			return wgm_iface_cmd_show(argc - 2, argv + 2, ctx);

		if (!strcmp(argv[2], "update"))
			return wgm_iface_cmd_update(argc - 2, argv + 2, ctx);

		printf("Error: unknown command: iface %s\n", argv[2]);
		return 1;
	}

	if (!strcmp(argv[1], "peer")) {
		if (argc == 2) {
			show_usage_peer(argv[0]);
			return 1;
		}

		if (!strcmp(argv[2], "add"))
			return wgm_peer_cmd_add(argc - 2, argv + 2, ctx);

		if (!strcmp(argv[2], "del"))
			return wgm_peer_cmd_del(argc - 2, argv + 2, ctx);

		if (!strcmp(argv[2], "show"))
			return wgm_peer_cmd_show(argc - 2, argv + 2, ctx);

		if (!strcmp(argv[2], "update"))
			return wgm_peer_cmd_update(argc - 2, argv + 2, ctx);

		printf("Error: unknown command: peer %s\n", argv[2]);
		return 1;
	}

	printf("Error: unknown command: %s\n", argv[1]);
	return 1;
}

int main(int argc, char *argv[])
{
	struct wgm_ctx ctx;
	int ret;

	if (argc == 1) {
		fprintf(stderr, "Usage: %s <command> [options]\n", argv[0]);
		return 1;
	}

	ret = wgm_init_ctx(&ctx);
	if (ret)
		return ret;

	ret = wgm_run(argc, argv, &ctx);
	wgm_free_ctx(&ctx);
	return ret;
}
