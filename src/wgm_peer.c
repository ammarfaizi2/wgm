// SPDX-License-Identifier: GPL-2.0-only

#include "wgm_peer.h"

struct wgm_peer_arg {
	char			ifname[IFNAMSIZ];
	char			public_key[256];
	char			endpoint[128];
	char			bind_ip[16];
	struct wgm_str_array	allowed_ips;
};

static const struct wgm_opt options[] = {
	#define PEER_ARG_DEV		(1ull << 0)
	{ PEER_ARG_DEV,		"dev",		required_argument,	NULL,	'd' },

	#define PEER_ARG_PUBLIC_KEY	(1ull << 1)
	{ PEER_ARG_PUBLIC_KEY,	"public-key",	required_argument,	NULL,	'p' },

	#define PEER_ARG_ENDPOINT	(1ull << 2)
	{ PEER_ARG_ENDPOINT,	"endpoint",	required_argument,	NULL,	'e' },

	#define PEER_ARG_BIND_IP	(1ull << 3)
	{ PEER_ARG_BIND_IP,	"bind-ip",	required_argument,	NULL,	'b' },

	#define PEER_ARG_ALLOWED_IPS	(1ull << 4)
	{ PEER_ARG_ALLOWED_IPS,	"allowed-ips",	required_argument,	NULL,	'a' },

	#define PEER_ARG_HELP		(1ull << 5)
	{ PEER_ARG_HELP,		"help",	no_argument,		NULL,	'h' },

	#define PEER_ARG_FORCE		(1ull << 6)
	{ PEER_ARG_FORCE,	"force",	no_argument,		NULL,	'f' },

	{ 0, NULL, 0, NULL, 0 }
};

static void wgm_peer_show_usage(void)
{
	printf("Usage: wgm peer [add|del|show|update] [OPTIONS]\n");
	printf("Commands:\n");
	printf("  add    - Add a new peer to the interface\n");
	printf("  del    - Delete a peer from the interface\n");
	printf("  show   - Show a peer configuration\n");
	printf("  update - Update a peer configuration\n");
	printf("  list   - List all peers on the interface\n");
	printf("Options:\n");
	printf("  -d, --dev         Interface name\n");
	printf("  -p, --public-key  Public key of the peer\n");
	printf("  -e, --endpoint    Endpoint of the peer\n");
	printf("  -b, --bind-ip     Bind IP of the peer\n");
	printf("  -a, --allowed-ips Allowed IPs of the peer\n");
	printf("  -f, --force       Force the operation\n");
	printf("  -h, --help        Show this help message\n");
}

static int wgm_iface_getopt(int argc, char *argv[], struct wgm_iface_arg *arg,
			    uint64_t allowed_args, uint64_t required_args,
			    uint64_t *out_args_p)
{
	struct option *long_opt;
	uint64_t out_args = 0;
	char *short_opt;
	int c, ret;
	size_t i;

	ret = wgm_create_getopt_long_args(&long_opt, &short_opt, options,
					  ARRAY_SIZE(options));
	if (ret)
		return ret;

out:
	wgm_free_getopt_long_args(long_opt, short_opt);
	return ret;
}

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
	wgm_peer_free(dst);
	memcpy(dst->public_key, src->public_key, sizeof(dst->public_key));
	memcpy(dst->bind_ip, src->bind_ip, sizeof(dst->bind_ip));
	return wgm_str_array_copy(&dst->allowed_ips, &src->allowed_ips);
}

void wgm_peer_move(struct wgm_peer *dst, struct wgm_peer *src)
{
	wgm_peer_free(dst);
	memcpy(dst->public_key, src->public_key, sizeof(dst->public_key));
	memcpy(dst->bind_ip, src->bind_ip, sizeof(dst->bind_ip));
	wgm_str_array_move(&dst->allowed_ips, &src->allowed_ips);
	memset(src, 0, sizeof(*src));
}

void wgm_peer_free(struct wgm_peer *peer)
{
	free(peer->allowed_ips.arr);
	memset(peer, 0, sizeof(*peer));
}
