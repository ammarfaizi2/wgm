// SPDX-License-Identifier: GPL-2.0-only

#include "wgm.h"
#include "wgm_peer.h"
#include "wgm_iface.h"

#include <stdlib.h>

static void show_usage(const char *app)
{
	printf("Usage: %s [COMMAND] [OPTIONS]\n\n", app);
	printf("Commands:\n");
	printf("  iface - Manage WireGuard interfaces\n");
	printf("  peer  - Manage WireGuard peers\n");
}

void show_usage_iface(const char *app, bool show_cmds)
{
	if (!app)
		app = "wgm";

	printf("Usage: %s iface [add|del|show|update|list] [OPTIONS]\n\n", app);
	if (show_cmds) {
		printf("Commands:\n");
		printf("  add    - Add a new WireGuard interface\n");
		printf("  del    - Delete an existing WireGuard interface\n");
		printf("  show   - Show information about a WireGuard interface\n");
		printf("  update - Update an existing WireGuard interface\n");
		printf("  list   - List all WireGuard interfaces (no options required)\n");
		printf("\n");
	}
	printf("Options:\n");
	printf("  -d, --dev <name>          Interface name\n");
	printf("  -l, --listen-port <port>  Listen port\n");
	printf("  -k, --private-key <key>   Private key\n");
	printf("  -a, --address <addr>      Interface address\n");
	printf("  -m, --mtu <size>          MTU size\n");
	printf("  -i, --allowed-ips <ips>   Allowed IPs\n");
	printf("  -h, --help                Show this help message\n");
	printf("  -f, --force               Force operation\n");
	printf("\n");
}

void show_usage_peer(const char *app, bool show_cmds)
{
	if (!app)
		app = "wgm";

	printf("Usage: wgm peer [add|del|show|update|list] [OPTIONS]\n\n");
	if (show_cmds) {
		printf("Commands:\n");
		printf("  add    - Add a new peer to a WireGuard interface\n");
		printf("  del    - Delete an existing peer from a WireGuard interface\n");
		printf("  show   - Show information about a peer in a WireGuard interface\n");
		printf("  update - Update an existing peer in a WireGuard interface\n");
		printf("  list   - List all peers in a WireGuard interface\n");
		printf("\n");
	}
	printf("Options:\n");
	printf("  -d, --dev         Interface name\n");
	printf("  -p, --public-key  Public key of the peer\n");
	printf("  -e, --endpoint    Endpoint of the peer\n");
	printf("  -b, --bind-ip     Bind IP of the peer\n");
	printf("  -a, --allowed-ips Allowed IPs of the peer\n");
	printf("  -f, --force       Force the operation\n");
	printf("  -h, --help        Show this help message\n");
	printf("\n");
}

static int wgm_ctx_init(struct wgm_ctx *ctx)
{
	const char *data_dir = getenv("WGM_DATA_DIR");
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

static void wgm_ctx_free(struct wgm_ctx *ctx)
{
	free(ctx->data_dir);
	memset(ctx, 0, sizeof(*ctx));
}

static int wgm_ctx_run(int argc, char *argv[], struct wgm_ctx *ctx)
{
	if (argc < 2) {
		fprintf(stderr, "Error: missing command\n");
		return 1;
	}

	if (strcmp(argv[1], "iface") == 0) {
		if (argc < 3) {
			show_usage_iface(argv[0], true);
			return 1;
		}

		if (!strcmp(argv[2], "add"))
			return wgm_iface_cmd_add(argc - 1, argv + 1, ctx);

		if (!strcmp(argv[2], "del"))
			return wgm_iface_cmd_del(argc - 1, argv + 1, ctx);

		if (!strcmp(argv[2], "show"))
			return wgm_iface_cmd_show(argc - 1, argv + 1, ctx);

		if (!strcmp(argv[2], "update"))
			return wgm_iface_cmd_update(argc - 1, argv + 1, ctx);

		if (!strcmp(argv[2], "list"))
			return wgm_iface_cmd_list(argc - 1, argv + 1, ctx);

		fprintf(stderr, "Error: unknown command: %s\n\n", argv[2]);
		show_usage_iface(argv[0], true);
		return 1;
	}

	if (strcmp(argv[1], "peer") == 0) {
		if (argc < 3) {
			show_usage_peer(argv[0], true);
			return 1;
		}

		if (!strcmp(argv[2], "add"))
			return wgm_peer_cmd_add(argc - 1, argv + 1, ctx);

		if (!strcmp(argv[2], "del"))
			return wgm_peer_cmd_del(argc - 1, argv + 1, ctx);

		if (!strcmp(argv[2], "show"))
			return wgm_peer_cmd_show(argc - 1, argv + 1, ctx);

		if (!strcmp(argv[2], "update"))
			return wgm_peer_cmd_update(argc - 1, argv + 1, ctx);

		if (!strcmp(argv[2], "list"))
			return wgm_peer_cmd_list(argc - 1, argv + 1, ctx);

		fprintf(stderr, "Error: unknown command: %s\n\n", argv[2]);
		show_usage_peer(argv[0], true);
		return 1;
	}

	fprintf(stderr, "Error: unknown command: %s\n\n", argv[1]);
	show_usage(argv[0]);
	return 1;
}

int main(int argc, char *argv[])
{
	struct wgm_ctx ctx;
	int ret;

	if (argc == 1) {
		show_usage(argv[0]);
		return 1;
	}

	ret = wgm_ctx_init(&ctx);
	if (ret)
		return ret;

	ret = wgm_ctx_run(argc, argv, &ctx);
	wgm_ctx_free(&ctx);
	return ret;
}
