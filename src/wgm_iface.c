// SPDX-License-Identifier: GPL-2.0-only
#include "wgm.h"
#include "wgm_iface.h"
#include "wgm_peer.h"
#include "wgm_conf.h"

#include <getopt.h>
#include <dirent.h>

struct wgm_iface_arg {
	bool			force;
	char			ifname[IFNAMSIZ];
	uint16_t		listen_port;
	uint16_t		mtu;
	char			private_key[256];
	struct wgm_str_array	addresses;
	struct wgm_str_array	allowed_ips;
};

static const struct wgm_opt options[] = {
	#define IFACE_ARG_DEV		(1ull << 0)
	{ IFACE_ARG_DEV,		"dev",		required_argument,	NULL,	'd' },

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

static void wgm_iface_show_usage(void)
{
	show_usage_iface(NULL, false);
}

int wgm_iface_opt_get_dev(char *ifname, size_t iflen, const char *dev)
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
			wgm_iface_show_usage();
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
			wgm_iface_show_usage();
			ret = -EINVAL;
			goto out;
		}

		if ((cid & required_args) && !(cid & out_args)) {
			wgm_log_err("Error: Option '--%s' is required\n\n", name);
			wgm_iface_show_usage();
			ret = -EINVAL;
			goto out;
		}
	}

	*out_args_p = out_args;
out:
	wgm_free_getopt_long_args(long_opt, short_opt);
	return ret;
}

static void wgm_peer_array_free(struct wgm_peer_array *peers)
{
	size_t i;

	for (i = 0; i < peers->nr; i++)
		wgm_peer_free(&peers->peers[i]);

	free(peers->peers);
	memset(peers, 0, sizeof(*peers));
}

static int wgm_peer_array_to_json(json_object **jobj, const struct wgm_peer_array *peers)
{
	json_object *jarr;
	size_t i;
	int ret;

	jarr = json_object_new_array();
	if (!jarr) {
		wgm_log_err("Error: wgm_peer_array_to_json: Failed to create JSON array\n");
		return -ENOMEM;
	}

	for (i = 0; i < peers->nr; i++) {
		json_object *jpeer;

		ret = wgm_peer_to_json(&jpeer, &peers->peers[i]);
		if (ret) {
			wgm_log_err("Error: wgm_peer_array_to_json: Failed to convert peer data to JSON\n");
			json_object_put(jarr);
			return ret;
		}

		json_object_array_add(jarr, jpeer);
	}

	*jobj = jarr;
	return 0;
}

static int wgm_peer_array_from_json(struct wgm_peer_array *peers, const json_object *jarr)
{
	size_t i, nr;
	int ret;

	nr = json_object_array_length(jarr);
	if (!nr) {
		peers->peers = NULL;
		peers->nr = 0;
		return 0;
	}

	peers->peers = malloc(nr * sizeof(*peers->peers));
	if (!peers->peers) {
		wgm_log_err("Error: wgm_peer_array_from_json: Failed to allocate memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < nr; i++) {
		const json_object *jpeer = json_object_array_get_idx(jarr, i);

		ret = wgm_peer_from_json(&peers->peers[i], jpeer);
		if (ret) {
			wgm_log_err("Error: wgm_peer_array_from_json: Failed to parse peer data\n");
			wgm_peer_array_free(peers);
			return ret;
		}
	}

	peers->nr = nr;
	return 0;
}

static int wgm_peer_array_copy(struct wgm_peer_array *dst, const struct wgm_peer_array *src)
{
	size_t i;
	int ret;

	dst->peers = calloc(src->nr, sizeof(*dst->peers));
	if (!dst->peers) {
		wgm_log_err("Error: wgm_peer_array_copy: Failed to allocate memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < src->nr; i++) {
		ret = wgm_peer_copy(&dst->peers[i], &src->peers[i]);
		if (ret) {
			wgm_log_err("Error: wgm_peer_array_copy: Failed to copy peer data\n");
			while (i--)
				wgm_peer_free(&dst->peers[i]);
			free(dst->peers);
			return ret;
		}
	}

	dst->nr = src->nr;
	return 0;
}

static char *wgm_iface_get_json_path(struct wgm_ctx *ctx, const char *devname)
{
	char *path;
	int ret;

	ret = wgm_asprintf(&path, "%s/json", ctx->data_dir);
	if (ret)
		return NULL;

	ret = mkdir_recursive(path, 0700);
	if (ret) {
		wgm_log_err("Error: wgm_iface_get_json_path: Failed to create directory '%s': %s\n", path, strerror(-ret));
		free(path);
		return NULL;
	}

	free(path);
	ret = wgm_asprintf(&path, "%s/json/%s.json", ctx->data_dir, devname);
	if (ret)
		return NULL;

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

	ret = wgm_str_array_from_json(arr, tmp);
	if (ret) {
		wgm_log_err("Error: load_key_array: Failed to parse JSON array\n");
		return ret;
	}

	return 0;
}

static int wgm_iface_load_from_json(struct wgm_iface *iface, const json_object *jobj)
{
	json_object *tmp;
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

	ret = json_object_object_get_ex(jobj, "peers", &tmp);
	if (!ret) {
		wgm_log_err("Error: wgm_iface_load_from_json: Missing 'peers' field\n");
		return -EINVAL;
	}

	if (!json_object_is_type(tmp, json_type_array)) {
		wgm_log_err("Error: wgm_iface_load_from_json: 'peers' field must be an array\n");
		return -EINVAL;
	}

	ret = wgm_peer_array_from_json(&iface->peers, tmp);
	if (ret) {
		wgm_log_err("Error: wgm_iface_load_from_json: Failed to parse 'peers' field\n");
		return ret;
	}

	return 0;
}

static int __wgm_iface_append_peer(struct wgm_iface *iface, const struct wgm_peer *peer)
{
	struct wgm_peer	*new_peers;
	size_t new_nr;
	int ret;

	new_nr = iface->peers.nr + 1;
	new_peers = realloc(iface->peers.peers, new_nr * sizeof(*new_peers));
	if (!new_peers) {
		wgm_log_err("Error: __wgm_iface_append_peer: Failed to allocate memory\n");
		return -ENOMEM;
	}

	iface->peers.peers = new_peers;
	ret = wgm_peer_copy(&new_peers[iface->peers.nr], peer);
	if (ret) {
		wgm_log_err("Error: __wgm_iface_append_peer: Failed to copy peer data\n");
		return ret;
	}

	iface->peers.nr = new_nr;
	return 0;
}

int wgm_iface_add_peer(struct wgm_iface *iface, const struct wgm_peer *peer, bool force_update)
{
	struct wgm_peer tmp;
	size_t i;
	int ret;

	memset(&tmp, 0, sizeof(tmp));
	for (i = 0; i < iface->peers.nr; i++) {
		if (strcmp(iface->peers.peers[i].public_key, peer->public_key))
			continue;

		if (!force_update) {
			wgm_log_err("Error: wgm_iface_add_peer: Peer with public key '%s' already exists, use --force to force update\n",
				    peer->public_key);
			return -EEXIST;
		}

		wgm_peer_move(&tmp, &iface->peers.peers[i]);
		ret = wgm_peer_copy(&iface->peers.peers[i], peer);
		if (ret) {
			wgm_log_err("Error: wgm_iface_add_peer: Failed to copy peer data\n");
			wgm_peer_move(&iface->peers.peers[i], &tmp);
			return ret;
		}

		wgm_peer_free(&tmp);
		return 0;
	}

	return __wgm_iface_append_peer(iface, peer);
}

int wgm_iface_del_peer(struct wgm_iface *iface, size_t idx)
{
	struct wgm_peer *new_peers;
	size_t new_nr;

	if (idx >= iface->peers.nr) {
		wgm_log_err("Error: wgm_iface_del_peer: Invalid peer index\n");
		return -EINVAL;
	}

	wgm_peer_free(&iface->peers.peers[idx]);

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
	bool found = false;
	size_t i;
	int ret;

	for (i = 0; i < iface->peers.nr; i++) {
		printf("iface->peers.peers[i].public_key: '%s'\n", iface->peers.peers[i].public_key);
		printf("pubkey: '%s'\n", pubkey);
		if (strcmp(iface->peers.peers[i].public_key, pubkey))
			continue;

		found = true;
		ret = wgm_iface_del_peer(iface, i);
		if (ret) {
			wgm_log_err("Error: wgm_iface_del_peer_by_pubkey: Failed to delete peer\n");
			return ret;
		}
	}

	if (!found) {
		wgm_log_err("Error: wgm_iface_del_peer_by_pubkey: Peer with public key '%s' not found\n", pubkey);
		return -ENOENT;
	}

	return 0;
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
	wgm_str_array_free(&iface->addresses);
	wgm_str_array_free(&iface->allowed_ips);
	wgm_peer_array_free(&iface->peers);
	memset(iface, 0, sizeof(*iface));
}

int wgm_iface_load(struct wgm_iface *iface, struct wgm_ctx *ctx, const char *devname)
{
	char *path, *jstr;
	json_object *jobj;
	size_t len;
	FILE *fp;
	int ret;

	path = wgm_iface_get_json_path(ctx, devname);
	if (!path)
		return -ENOMEM;

	fp = fopen(path, "rb");
	if (!fp) {
		ret = -errno;
		goto out_free_path;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (!len) {
		ret = -ENOENT;
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

int wgm_iface_to_json(json_object **jobj, const struct wgm_iface *iface)
{
	json_object *jarr;
	int ret;

	*jobj = json_object_new_object();
	if (!*jobj) {
		wgm_log_err("Error: wgm_iface_to_json: Failed to create JSON object\n");
		return -ENOMEM;
	}

	json_object_object_add(*jobj, "dev", json_object_new_string(iface->ifname));
	json_object_object_add(*jobj, "listen-port", json_object_new_int(iface->listen_port));
	json_object_object_add(*jobj, "private-key", json_object_new_string(iface->private_key));
	json_object_object_add(*jobj, "mtu", json_object_new_int(iface->mtu));

	ret = wgm_str_array_to_json(&jarr, &iface->addresses);
	if (ret) {
		wgm_log_err("Error: wgm_iface_to_json: Failed to convert 'address' array to JSON\n");
		json_object_put(*jobj);
		return ret;
	}
	json_object_object_add(*jobj, "address", jarr);

	ret = wgm_str_array_to_json(&jarr, &iface->allowed_ips);
	if (ret) {
		wgm_log_err("Error: wgm_iface_to_json: Failed to convert 'allowed-ips' array to JSON\n");
		json_object_put(*jobj);
		return ret;
	}
	json_object_object_add(*jobj, "allowed-ips", jarr);

	ret = wgm_peer_array_to_json(&jarr, &iface->peers);
	if (ret) {
		wgm_log_err("Error: wgm_iface_to_json: Failed to convert 'peers' array to JSON\n");
		json_object_put(*jobj);
		return ret;
	}
	json_object_object_add(*jobj, "peers", jarr);

	return 0;
}

static char *wgm_iface_to_json_str(const struct wgm_iface *iface)
{
	json_object *jobj;
	const char *tmp;
	char *jstr;
	int ret;

	ret = wgm_iface_to_json(&jobj, iface);
	if (ret) {
		wgm_log_err("Error: wgm_iface_to_json_str: Failed to convert interface data to JSON\n");
		return NULL;
	}

	tmp = json_object_to_json_string_ext(jobj, WGM_JSON_FLAGS);
	if (!tmp) {
		wgm_log_err("Error: wgm_iface_to_json_str: Failed to convert JSON object to string\n");
		json_object_put(jobj);
		return NULL;
	}

	jstr = strdup(tmp);
	json_object_put(jobj);
	if (!jstr)
		wgm_log_err("Error: wgm_iface_to_json_str: Failed to allocate memory\n");

	return jstr;
}

int wgm_iface_del(const struct wgm_iface *iface, struct wgm_ctx *ctx)
{
	char *path;
	int ret;

	path = wgm_iface_get_json_path(ctx, iface->ifname);
	if (!path)
		return -ENOMEM;

	ret = remove(path);
	if (ret) {
		ret = -errno;
		wgm_log_err("Error: wgm_iface_del: Failed to delete file '%s': %s\n", path, strerror(-ret));
	}

	free(path);
	return ret;
}

int wgm_iface_save(const struct wgm_iface *iface, struct wgm_ctx *ctx)
{
	char *path, *jstr;
	FILE *fp;

	path = wgm_iface_get_json_path(ctx, iface->ifname);
	if (!path)
		return -ENOMEM;

	fp = fopen(path, "wb");
	if (!fp) {
		int ret = -errno;
		wgm_log_err("Error: wgm_iface_save: Failed to open file '%s': %s\n", path, strerror(-ret));
		free(path);
		return ret;
	}

	jstr = wgm_iface_to_json_str(iface);
	if (!jstr) {
		wgm_log_err("Error: wgm_iface_save: Failed to convert interface data to JSON\n");
		fclose(fp);
		free(path);
		return -ENOMEM;
	}

	fputs(jstr, fp);
	fputc('\n', fp);

	free(jstr);
	fclose(fp);
	free(path);
	return wgm_conf_save(iface, ctx);
}

static void move_arg_to_iface(struct wgm_iface *iface, struct wgm_iface_arg *arg, uint64_t args)
{
	if (args & IFACE_ARG_DEV)
		strncpyl(iface->ifname, arg->ifname, sizeof(iface->ifname));

	if (args & IFACE_ARG_LISTEN_PORT)
		iface->listen_port = arg->listen_port;

	if (args & IFACE_ARG_PRIVATE_KEY)
		strncpyl(iface->private_key, arg->private_key, sizeof(iface->private_key));

	if (args & IFACE_ARG_ADDRESS)
		wgm_str_array_move(&iface->addresses, &arg->addresses);

	if (args & IFACE_ARG_MTU)
		iface->mtu = arg->mtu;

	if (args & IFACE_ARG_ALLOWED_IPS)
		wgm_str_array_move(&iface->allowed_ips, &arg->allowed_ips);
}

static void wgm_iface_free_arg(struct wgm_iface_arg *arg)
{
	wgm_str_array_free(&arg->addresses);
	wgm_str_array_free(&arg->allowed_ips);
	memset(arg, 0, sizeof(*arg));
}

static int apply_iface(struct wgm_iface *iface, struct wgm_iface_arg *arg,
		       uint64_t out_args, struct wgm_ctx *ctx)
{
	int ret;

	move_arg_to_iface(iface, arg, out_args);
	ret = wgm_iface_save(iface, ctx);
	if (ret) {
		wgm_log_err("Error: apply_iface: Failed to save interface data: %s\n", strerror(-ret));
		return ret;
	}

	return 0;
}

void wgm_iface_dump_json(const struct wgm_iface *iface)
{
	char *jstr;

	jstr = wgm_iface_to_json_str(iface);
	if (!jstr) {
		wgm_log_err("Error: wgm_iface_dump_json: Failed to convert interface data to JSON\n");
		return;
	}

	printf("%s\n", jstr);
	free(jstr);
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

	ret = wgm_iface_getopt(argc, argv, &arg, allowed_args, req_args, &out_args);
	if (ret)
		return ret;

	ret = wgm_iface_load(&iface, ctx, arg.ifname);
	if (!ret) {
		if (!arg.force) {
			wgm_log_err("Error: wgm_iface_cmd_add: Interface '%s' already exists, use --force to force update\n", arg.ifname);
			ret = -EEXIST;
			goto out;
		}
	} else {
		if (ret != -ENOENT) {
			wgm_log_err("Error: wgm_iface_cmd_add: Failed to load interface '%s': %s\n", arg.ifname, strerror(-ret));
			goto out;
		}
	}

	ret = apply_iface(&iface, &arg, out_args, ctx);
	wgm_iface_dump_json(&iface);
out:
	wgm_iface_free(&iface);
	wgm_iface_free_arg(&arg);
	return ret;
}

int wgm_iface_cmd_update(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t req_args = IFACE_ARG_DEV;
	static const uint64_t allowed_args = req_args | IFACE_ARG_LISTEN_PORT |
					      IFACE_ARG_PRIVATE_KEY | IFACE_ARG_ADDRESS |
					      IFACE_ARG_MTU | IFACE_ARG_ALLOWED_IPS |
					      IFACE_ARG_HELP | IFACE_ARG_FORCE;

	struct wgm_iface_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));
	ret = wgm_iface_getopt(argc, argv, &arg, allowed_args, req_args, &out_args);
	if (ret)
		return ret;

	ret = wgm_iface_load(&iface, ctx, arg.ifname);
	if (ret) {
		wgm_log_err("Error: wgm_iface_cmd_update: Failed to load interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	ret = apply_iface(&iface, &arg, out_args, ctx);
	wgm_iface_dump_json(&iface);
out:
	wgm_iface_free(&iface);
	wgm_iface_free_arg(&arg);
	return ret;
}

int wgm_iface_cmd_del(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t req_args = IFACE_ARG_DEV;
	static const uint64_t allowed_args = req_args | IFACE_ARG_HELP | IFACE_ARG_FORCE;

	struct wgm_iface_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));

	ret = wgm_iface_getopt(argc, argv, &arg, allowed_args, req_args, &out_args);
	if (ret)
		return ret;

	ret = wgm_iface_load(&iface, ctx, arg.ifname);
	if (ret) {
		if (arg.force)
			ret = 0;
		else
			wgm_log_err("Error: wgm_iface_cmd_del: Failed to load interface '%s' (use --force to silently ignore): %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

out:
	wgm_iface_free(&iface);
	wgm_iface_free_arg(&arg);
	return ret;
}

int wgm_iface_cmd_show(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t req_args = IFACE_ARG_DEV;
	static const uint64_t allowed_args = req_args | IFACE_ARG_HELP;

	struct wgm_iface_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));

	ret = wgm_iface_getopt(argc, argv, &arg, allowed_args, req_args, &out_args);
	if (ret)
		return ret;

	ret = wgm_iface_load(&iface, ctx, arg.ifname);
	if (ret) {
		wgm_log_err("Error: wgm_iface_cmd_show: Failed to load interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	wgm_iface_dump_json(&iface);
	ret = 0;
out:
	wgm_iface_free(&iface);
	wgm_iface_free_arg(&arg);
	return ret;
}

int wgm_iface_cmd_list(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t allowed_args = IFACE_ARG_HELP;

	struct wgm_iface_array ifaces;
	struct wgm_iface_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	struct dirent *ent;
	DIR *dir;
	int ret;

	memset(&ifaces, 0, sizeof(ifaces));
	memset(&iface, 0, sizeof(iface));
	memset(&arg, 0, sizeof(arg));

	ret = wgm_iface_getopt(argc, argv, &arg, allowed_args, 0, &out_args);
	if (ret)
		return ret;

	dir = opendir(ctx->data_dir);
	if (!dir) {
		ret = -errno;
		wgm_log_err("Error: wgm_iface_cmd_list: Failed to open directory '%s': %s\n", ctx->data_dir, strerror(-ret));
		return ret;
	}

	while (1) {
		size_t len;
		char *name;

		ent = readdir(dir);
		if (!ent)
			break;

		if (ent->d_type != DT_REG)
			continue;

		name = ent->d_name;

		/*
		 * Ensure the file ends with '.json'.
		 */
		len = strlen(name);
		if (len < 5 || strcmp(name + len - 5, ".json"))
			continue;

		/*
		 * Remove the '.json' extension.
		 */
		name[len - 5] = '\0';

		ret = wgm_iface_load(&iface, ctx, name);
		if (ret) {
			wgm_log_err("Error: wgm_iface_cmd_list: Failed to load interface '%s': %s\n", ent->d_name, strerror(-ret));
			break;
		}

		ret = wgm_iface_array_add(&ifaces, &iface);
		wgm_iface_free(&iface);
		if (ret) {
			wgm_log_err("Error: wgm_iface_cmd_list: Failed to add interface to array\n");
			break;
		}
	}

	closedir(dir);

	if (!ret)
		wgm_iface_array_dump_json(&ifaces);

	wgm_iface_array_free(&ifaces);
	return ret;
}

int wgm_iface_array_add(struct wgm_iface_array *ifaces, const struct wgm_iface *iface)
{
	struct wgm_iface *new_ifaces;
	size_t new_nr;
	int ret;

	new_nr = ifaces->nr + 1;
	new_ifaces = realloc(ifaces->ifaces, new_nr * sizeof(*new_ifaces));
	if (!new_ifaces) {
		wgm_log_err("Error: wgm_iface_array_add: Failed to allocate memory\n");
		return -ENOMEM;
	}

	ifaces->ifaces = new_ifaces;
	ret = wgm_iface_copy(&new_ifaces[ifaces->nr], iface);
	if (ret) {
		wgm_log_err("Error: wgm_iface_array_add: Failed to copy interface data\n");
		return ret;
	}

	ifaces->nr = new_nr;
	return 0;
}

int wgm_iface_array_to_json(json_object **jobj, const struct wgm_iface_array *ifaces)
{
	json_object *jarr;
	size_t i;
	int ret;

	jarr = json_object_new_array();
	if (!jarr) {
		wgm_log_err("Error: wgm_iface_array_to_json: Failed to create JSON array\n");
		return -ENOMEM;
	}

	for (i = 0; i < ifaces->nr; i++) {
		json_object *jiface;

		ret = wgm_iface_to_json(&jiface, &ifaces->ifaces[i]);
		if (ret) {
			wgm_log_err("Error: wgm_iface_array_to_json: Failed to convert interface data to JSON\n");
			json_object_put(jarr);
			return ret;
		}

		json_object_array_add(jarr, jiface);
	}

	*jobj = jarr;
	return 0;
}

int wgm_iface_copy(struct wgm_iface *dst, const struct wgm_iface *src)
{
	int ret;

	memset(dst, 0, sizeof(*dst));
	memcpy(dst->ifname, src->ifname, sizeof(dst->ifname));
	dst->listen_port = src->listen_port;
	dst->mtu = src->mtu;
	memcpy(dst->private_key, src->private_key, sizeof(dst->private_key));

	ret = wgm_str_array_copy(&dst->addresses, &src->addresses);
	if (ret) {
		memset(dst, 0, sizeof(*dst));
		wgm_log_err("Error: wgm_iface_copy: Failed to copy 'addresses' array\n");
		return ret;
	}

	ret = wgm_str_array_copy(&dst->allowed_ips, &src->allowed_ips);
	if (ret) {
		wgm_str_array_free(&dst->addresses);
		memset(dst, 0, sizeof(*dst));
		wgm_log_err("Error: wgm_iface_copy: Failed to copy 'allowed-ips' array\n");
		return ret;
	}

	ret = wgm_peer_array_copy(&dst->peers, &src->peers);
	if (ret) {
		wgm_str_array_free(&dst->addresses);
		wgm_str_array_free(&dst->allowed_ips);
		memset(dst, 0, sizeof(*dst));
		wgm_log_err("Error: wgm_iface_copy: Failed to copy 'peers' array\n");
		return ret;
	}

	return 0;
}

void wgm_iface_move(struct wgm_iface *dst, struct wgm_iface *src)
{
	wgm_iface_free(dst);
	memcpy(dst, src, sizeof(*dst));
	memset(src, 0, sizeof(*src));
}

void wgm_iface_array_dump_json(const struct wgm_iface_array *ifaces)
{
	json_object *jobj;
	int ret;

	ret = wgm_iface_array_to_json(&jobj, ifaces);
	if (ret) {
		wgm_log_err("Error: wgm_iface_array_dump_json: Failed to convert interface array to JSON\n");
		return;
	}

	printf("%s\n", json_object_to_json_string_ext(jobj, WGM_JSON_FLAGS));
	json_object_put(jobj);
}

void wgm_iface_array_free(struct wgm_iface_array *ifaces)
{
	size_t i;

	for (i = 0; i < ifaces->nr; i++)
		wgm_iface_free(&ifaces->ifaces[i]);

	free(ifaces->ifaces);
	memset(ifaces, 0, sizeof(*ifaces));
}

void wgm_iface_peer_array_dump_json(const struct wgm_peer_array *peers)
{
	json_object *jobj;
	int ret;

	ret = wgm_peer_array_to_json(&jobj, peers);
	if (ret) {
		wgm_log_err("Error: wgm_iface_peer_array_dump_json: Failed to convert peer array to JSON\n");
		return;
	}

	printf("%s\n", json_object_to_json_string_ext(jobj, WGM_JSON_FLAGS));
	json_object_put(jobj);
}
