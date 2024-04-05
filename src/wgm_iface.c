// SPDX-License-Identifier: GPL-2.0-only
#include "wgm_iface.h"

#include <getopt.h>

struct wgm_iface_arg {
	bool			force;
	char			ifname[IFNAMSIZ];
	uint16_t		listen_port;
	uint16_t		mtu;
	char			private_key[128];
	struct wgm_str_array	addresses;
	struct wgm_str_array	allowed_ips;
};

static const struct wgm_opt options[] = {
	#define IFACE_ARG_DEV		(1ull << 0)
	{ IFACE_ARG_DEV,	"dev",		required_argument,	NULL,	'd' },

	#define IFACE_ARG_LISTEN_PORT	(1ull << 1)
	{ IFACE_ARG_LISTEN_PORT,	"listen-port",	required_argument,	NULL,	'l' },

	#define IFACE_ARG_PRIVATE_KEY	(1ull << 2)
	{ IFACE_ARG_PRIVATE_KEY,	"private-key",	required_argument,	NULL,	'k' },

	#define IFACE_ARG_ADDRESS	(1ull << 3)
	{ IFACE_ARG_ADDRESS,		"address",	required_argument,	NULL,	'a' },

	#define IFACE_ARG_MTU		(1ull << 4)
	{ IFACE_ARG_MTU,		"mtu",		required_argument,	NULL,	'm' },

	#define IFACE_ARG_ALLOWED_IPS	(1ull << 5)
	{ IFACE_ARG_ALLOWED_IPS,	"allowed-ips",	required_argument,	NULL,	'i' },

	#define IFACE_ARG_HELP		(1ull << 6)
	{ IFACE_ARG_HELP,		"help",		no_argument,		NULL,	'h' },

	#define IFACE_ARG_FORCE		(1ull << 7)
	{ IFACE_ARG_FORCE,		"force",	no_argument,		NULL,	'f' },

	{ 0, NULL, 0, NULL, 0 }
};

static void wgm_iface_show_usage(const char *app)
{
	printf("Usage: iface <command> [options]\n");
	printf("\n");
	printf("Commands:\n");
	printf("  add    - Add a new WireGuard interface\n");
	printf("  del    - Delete an existing WireGuard interface\n");
	printf("  show   - Show information about a WireGuard interface\n");
	printf("  update - Update an existing WireGuard interface\n");
	printf("\n");
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

static int wgm_iface_opt_get_dev(char *ifname, size_t iflen, const char *dev)
{
	size_t i;

	i = strlen(dev);
	if (i >= iflen) {
		wgm_log_err("Error: Interface name is too long, max %u characters\n", IFNAMSIZ - 1);
		return -EINVAL;
	}

	if (!i) {
		wgm_log_err("Error: Interface name cannot be empty\n");
		return -EINVAL;
	}

	/*
	 * Only allow alphanumeric and hyphen characters.
	 */
	for (i = 0; dev[i]; i++) {
		if (!isalnum(dev[i]) && dev[i] != '-') {
			wgm_log_err("Error: Interface name can only contain alphanumeric and hyphen characters\n");
			return -EINVAL;
		}
	}

	strncpyl(ifname, dev, iflen);
	return 0;
}

static int wgm_iface_opt_get_listen_port(uint16_t *port, const char *listen_port)
{
	unsigned long p;
	char *endptr;

	p = strtoul(listen_port, &endptr, 10);
	if (*endptr) {
		wgm_log_err("Error: Invalid listen port\n");
		return -EINVAL;
	}

	if (p > UINT16_MAX) {
		wgm_log_err("Error: Listen port is too large\n");
		return -EINVAL;
	}

	*port = p;
	return 0;
}

static int wgm_iface_opt_get_private_key(char *private_key, size_t keylen,
					 const char *key)
{
	size_t i;

	i = strlen(key);
	if (i >= keylen) {
		wgm_log_err("Error: Private key is too long, max %zu characters\n",
			    keylen - 1);
		return -EINVAL;
	}

	if (!i) {
		wgm_log_err("Error: Private key cannot be empty\n");
		return -EINVAL;
	}

	strncpyl(private_key, key, keylen);
	return 0;
}

static int wgm_iface_opt_get_address(struct wgm_str_array *addresses, const char *address)
{
	return wgm_parse_csv(addresses, address);
}

static int wgm_iface_opt_get_mtu(uint16_t *mtu, const char *mtu_str)
{
	unsigned long m;
	char *endptr;

	m = strtoul(mtu_str, &endptr, 10);
	if (*endptr) {
		wgm_log_err("Error: Invalid MTU size\n");
		return -EINVAL;
	}

	if (m > UINT16_MAX) {
		wgm_log_err("Error: MTU size is too large\n");
		return -EINVAL;
	}

	*mtu = m;
	return 0;
}

static int wgm_iface_opt_get_allowed_ips(struct wgm_str_array *allowed_ips, const char *ips)
{
	return wgm_parse_csv(allowed_ips, ips);
}

static int wgm_iface_getopt(int argc, char *argv[], struct wgm_ctx *ctx,
			    struct wgm_iface_arg *arg, uint64_t allowed_args,
			    uint64_t required_args, uint64_t *out_args_p)
{
	struct option *long_opt;
	uint64_t out_args = 0;
	char *short_opt;
	int c, ret;

	ret = wgm_create_getopt_long_args(&long_opt, &short_opt, options,
					  ARRAY_SIZE(options));
	if (ret)
		return ret;

	while (1) {
		c = getopt_long(argc, argv, short_opt, long_opt, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'd':
			if (wgm_iface_opt_get_dev(arg->ifname, sizeof(arg->ifname), optarg))
				return -EINVAL;
			out_args |= IFACE_ARG_DEV;
			break;
		case 'l':
			if (wgm_iface_opt_get_listen_port(&arg->listen_port, optarg))
				return -EINVAL;
			out_args |= IFACE_ARG_LISTEN_PORT;
			break;
		case 'k':
			if (wgm_iface_opt_get_private_key(arg->private_key, sizeof(arg->private_key), optarg))
				return -EINVAL;
			out_args |= IFACE_ARG_PRIVATE_KEY;
			break;
		case 'a':
			if (wgm_iface_opt_get_address(&arg->addresses, optarg))
				return -EINVAL;
			out_args |= IFACE_ARG_ADDRESS;
			break;
		case 'm':
			if (wgm_iface_opt_get_mtu(&arg->mtu, optarg))
				return -EINVAL;
			out_args |= IFACE_ARG_MTU;
			break;
		case 'i':
			if (wgm_iface_opt_get_allowed_ips(&arg->allowed_ips, optarg))
				return -EINVAL;
			out_args |= IFACE_ARG_ALLOWED_IPS;
			break;
		case 'h':
			wgm_iface_show_usage(argv[0]);
			out_args |= IFACE_ARG_HELP;
			ret = 1;
			goto out;
		case 'f':
			arg->force = true;
			out_args |= IFACE_ARG_FORCE;
			break;
		default:
			wgm_log_err("Error: Invalid option '%c'\n", c);
			ret = -EINVAL;
			goto out;
		}
	}

	*out_args_p = out_args;
out:
	wgm_free_getopt_long_args(long_opt, short_opt);
	return ret;
}

int wgm_iface_cmd_add(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t req_args = IFACE_ARG_DEV | IFACE_ARG_LISTEN_PORT |
					  IFACE_ARG_PRIVATE_KEY | IFACE_ARG_ADDRESS |
					  IFACE_ARG_MTU | IFACE_ARG_ALLOWED_IPS;
	static const uint64_t allowed_args = req_args | IFACE_ARG_HELP | IFACE_ARG_FORCE;

	struct wgm_iface_arg arg;
	uint64_t out_args = 0;
	int ret;

	memset(&arg, 0, sizeof(arg));
	ret = wgm_iface_getopt(argc, argv, ctx, &arg, allowed_args, req_args, &out_args);
	if (ret)
		return ret;

	
}

int wgm_iface_cmd_del(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}

int wgm_iface_cmd_show(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}

int wgm_iface_cmd_update(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}
