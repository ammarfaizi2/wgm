// SPDX-License-Identifier: GPL-2.0-only
#include "wgm.h"
#include "wgm_iface.h"
#include "wgm_peer.h"

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
	size_t i;

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
			out_args |= IFACE_ARG_HELP;
			wgm_iface_show_usage(argv[0]);
			ret = -1;
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

	for (i = 0; i < ARRAY_SIZE(options); i++) {
		const char *name = options[i].name;
		uint64_t cid = options[i].id;

		if ((cid & out_args) && !(cid & allowed_args)) {
			wgm_log_err("Error: Option '--%s' is not allowed\n\n", name);
			wgm_iface_show_usage(argv[0]);
			ret = -EINVAL;
			goto out;
		}

		if ((cid & required_args) && !(cid & out_args)) {
			wgm_log_err("Error: Option '--%s' is required\n\n", name);
			wgm_iface_show_usage(argv[0]);
			ret = -EINVAL;
			goto out;
		}
	}

	*out_args_p = out_args;
out:
	wgm_free_getopt_long_args(long_opt, short_opt);
	return ret;
}

static char *wgm_iface_get_file_path(struct wgm_ctx *ctx, const char *devname)
{
	char *path;
	size_t len;

	len = strlen(ctx->data_dir) + strlen(devname) + 2;
	path = malloc(len);
	if (!path) {
		wgm_log_err("Error: wgm_iface_get_file_path: Failed to allocate memory\n");
		return NULL;
	}

	snprintf(path, len, "%s/%s", ctx->data_dir, devname);
	return path;
}

static const char *load_key_str(const json_object *jobj, const char *key)
{
	json_object *tmp;

	tmp = json_object_object_get(jobj, key);
	if (!tmp || !json_object_is_type(tmp, json_type_string))
		return NULL;

	return json_object_get_string(tmp);
}

static int load_key_int(const json_object *jobj, const char *key, int *val)
{
	json_object *tmp;

	tmp = json_object_object_get(jobj, key);
	if (!tmp || !json_object_is_type(tmp, json_type_int))
		return -EINVAL;

	*val = json_object_get_int(tmp);
	return 0;
}

static int load_key_array(const json_object *jobj, const char *key, struct wgm_str_array *arr)
{
	json_object *tmp;
	int ret;

	tmp = json_object_object_get(jobj, key);
	if (!tmp || !json_object_is_type(tmp, json_type_array))
		return -EINVAL;

	ret = wgm_json_to_str_array(arr, tmp);
	if (ret) {
		wgm_log_err("Error: load_key_array: Failed to parse JSON array\n");
		return ret;
	}

	return 0;
}

static int wgm_iface_load_from_json(struct wgm_iface *iface, const json_object *jobj)
{
	const char *stmp;
	int itmp;
	int ret;

	stmp = load_key_str(jobj, "dev");
	if (!stmp) {
		wgm_log_err("Error: wgm_iface_load_from_json: Missing 'dev' field (string)\n");
		return -EINVAL;
	}

	if (wgm_iface_opt_get_dev(iface->ifname, sizeof(iface->ifname), stmp))
		return -EINVAL;

	ret = load_key_int(jobj, "listen-port", &itmp);
	if (ret) {
		wgm_log_err("Error: wgm_iface_load_from_json: Missing 'listen-port' field (int)\n");
		return ret;
	}

	if (itmp < 0 || itmp > UINT16_MAX) {
		wgm_log_err("Error: wgm_iface_load_from_json: Invalid 'listen-port' value, must be in range [0, %u]\n", UINT16_MAX);
		return -EINVAL;
	}

	iface->listen_port = (uint16_t)itmp;

	stmp = load_key_str(jobj, "private-key");
	if (!stmp) {
		wgm_log_err("Error: wgm_iface_load_from_json: Missing 'private-key' field (string)\n");
		return -EINVAL;
	}

	if (wgm_iface_opt_get_private_key(iface->private_key, sizeof(iface->private_key), stmp))
		return -EINVAL;

	ret = load_key_array(jobj, "address", &iface->addresses);
	if (ret) {
		wgm_log_err("Error: wgm_iface_load_from_json: Missing 'address' field (array of strings)\n");
		return ret;
	}

	ret = load_key_int(jobj, "mtu", &itmp);
	if (ret) {
		wgm_log_err("Error: wgm_iface_load_from_json: Missing 'mtu' field (int)\n");
		return ret;
	}

	if (itmp < 0 || itmp > UINT16_MAX) {
		wgm_log_err("Error: wgm_iface_load_from_json: Invalid 'mtu' value, must be in range [0, %u]\n", UINT16_MAX);
		return -EINVAL;
	}

	iface->mtu = (uint16_t)itmp;

	ret = load_key_array(jobj, "allowed-ips", &iface->allowed_ips);
	if (ret) {
		wgm_log_err("Error: wgm_iface_load_from_json: Missing 'allowed-ips' field (array of strings)\n");
		return ret;
	}

	return 0;
}

int wgm_iface_add_peer(struct wgm_iface *iface, const struct wgm_peer *peer)
{
	struct wgm_peer	*new_peers;
	size_t new_nr;
	int ret;

	new_nr = iface->peers.nr + 1;
	new_peers = realloc(iface->peers.peers, new_nr * sizeof(*new_peers));
	if (!new_peers) {
		wgm_log_err("Error: wgm_iface_add_peer: Failed to allocate memory\n");
		return -ENOMEM;
	}

	iface->peers.peers = new_peers;
	ret = wgm_peer_copy(&new_peers[iface->peers.nr], peer);
	if (ret) {
		wgm_log_err("Error: wgm_iface_add_peer: Failed to copy peer data\n");
		return ret;
	}

	iface->peers.nr = new_nr;
	return 0;
}

int wgm_iface_del_peer(struct wgm_iface *iface, size_t idx)
{
	struct wgm_peer *new_peers;
	size_t new_nr;

	if (idx >= iface->peers.nr) {
		wgm_log_err("Error: wgm_iface_del_peer: Invalid peer index\n");
		return -EINVAL;
	}

	wgm_free_peer(&iface->peers.peers[idx]);

	new_nr = iface->peers.nr - 1;
	if (!new_nr) {
		free(iface->peers.peers);
		iface->peers.peers = NULL;
		iface->peers.nr = 0;
		return 0;
	}

	memmove(&iface->peers.peers[idx], &iface->peers.peers[idx + 1],
		(new_nr - idx) * sizeof(*iface->peers.peers));

	iface->peers.nr = new_nr;
	new_peers = realloc(iface->peers.peers, new_nr * sizeof(*new_peers));
	if (new_peers)
		iface->peers.peers = new_peers;

	return 0;
}

int wgm_iface_del_peer_by_pubkey(struct wgm_iface *iface, const char *pubkey)
{
	size_t i;
	int ret;

	for (i = 0; i < iface->peers.nr; i++) {
		if (strcmp(iface->peers.peers[i].public_key, pubkey))
			continue;

		ret = wgm_iface_del_peer(iface, i);
		if (ret) {
			wgm_log_err("Error: wgm_iface_del_peer_by_pubkey: Failed to delete peer\n");
			return ret;
		}
	}

	wgm_log_err("Error: wgm_iface_del_peer_by_pubkey: Peer with public key '%s' not found\n", pubkey);
	return -ENOENT;
}

int wgm_iface_get_peer_by_pubkey(const struct wgm_iface *iface, const char *pubkey,
				 struct wgm_peer **peer)
{
	size_t i;

	for (i = 0; i < iface->peers.nr; i++) {
		if (!strcmp(iface->peers.peers[i].public_key, pubkey)) {
			*peer = &iface->peers.peers[i];
			return 0;
		}
	}

	wgm_log_err("Error: wgm_iface_get_peer_by_pubkey: Peer with public key '%s' not found\n", pubkey);
	return -ENOENT;
}

void wgm_iface_free(struct wgm_iface *iface)
{
	wgm_free_str_array(&iface->addresses);
	wgm_free_str_array(&iface->allowed_ips);
	memset(iface, 0, sizeof(*iface));
}

int wgm_iface_load(struct wgm_iface *iface, struct wgm_ctx *ctx, const char *devname)
{
	char *path, *jstr;
	json_object *jobj;
	size_t len;
	FILE *fp;
	int ret;

	path = wgm_iface_get_file_path(ctx, devname);
	if (!path)
		return -ENOMEM;

	fp = fopen(path, "rb");
	if (!fp) {
		ret = -errno;
		wgm_log_err("Error: wgm_iface_load: Failed to open file '%s': %s\n", path, strerror(-ret));
		goto out_free_path;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (!len) {
		ret = -EINVAL;
		wgm_log_err("Error: wgm_iface_load: File '%s' is empty\n", path);
		goto out_free_fp;
	}

	jstr = malloc(len + 1);
	if (!jstr) {
		ret = -ENOMEM;
		wgm_log_err("Error: wgm_iface_load: Failed to allocate memory\n");
		goto out_free_fp;
	}

	if (fread(jstr, 1, len, fp) != len) {
		ret = -EIO;
		wgm_log_err("Error: wgm_iface_load: Failed to read file '%s': %s\n", path, strerror(-ret));
		goto out_free_jstr;
	}

	jstr[len] = '\0';
	jobj = json_tokener_parse(jstr);
	if (!jobj) {
		ret = -EINVAL;
		wgm_log_err("Error: wgm_iface_load: Failed to parse JSON data\n");
		goto out_free_jstr;
	}

	ret = wgm_iface_load_from_json(iface, jobj);
	json_object_put(jobj);
out_free_jstr:
	free(jstr);
out_free_fp:
	fclose(fp);
out_free_path:
	free(path);
	return ret;
}

int wgm_iface_cmd_add(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t req_args = IFACE_ARG_DEV | IFACE_ARG_LISTEN_PORT |
					  IFACE_ARG_PRIVATE_KEY | IFACE_ARG_ADDRESS |
					  IFACE_ARG_MTU | IFACE_ARG_ALLOWED_IPS;
	static const uint64_t allowed_args = req_args | IFACE_ARG_HELP | IFACE_ARG_FORCE;

	struct wgm_iface_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));

	ret = wgm_iface_getopt(argc, argv, ctx, &arg, allowed_args, req_args, &out_args);
	if (ret)
		return ret;

	ret = wgm_iface_load(&iface, ctx, arg.ifname);
	if (ret)
		return ret;

	wgm_iface_free(&iface);
	return 0;
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
