// SPDX-License-Identifier: GPL-2.0-only

#include "wgm.h"
#include "wg_iface.h"
#include "wg_peer.h"

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

int main(int argc, char *argv[])
{
	if (argc == 1) {
		fprintf(stderr, "Usage: %s <command> [options]\n", argv[0]);
		return 1;
	}

	if (!strcmp(argv[1], "iface")) {
		if (argc == 2) {
			show_usage_iface(argv[0]);
			return 1;
		}

		if (!strcmp(argv[2], "add"))
			return wgm_iface_add(argc - 2, argv + 2);
	}

	if (!strcmp(argv[1], "peer")) {
		if (argc == 2) {
			show_usage_peer(argv[0]);
			return 1;
		}
	}

	printf("Error: unknown command: %s\n", argv[1]);
	return 1;
}
