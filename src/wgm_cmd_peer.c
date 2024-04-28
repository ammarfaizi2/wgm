// SPDX-License-Identifier: GPL-2.0-only

#include <string.h>

#include "wgm_cmd_peer.h"

static const struct wgm_opt options[] = {
	#define PEER_OPT_DEV			(1ull << 0ull)
	{ PEER_OPT_DEV,			"dev",		required_argument,	NULL,	'd' },

	#define PEER_OPT_PRIVATE_KEY		(1ull << 1ull)
	{ PEER_OPT_PRIVATE_KEY,		"private-key",	required_argument,	NULL,	'k' },

	#define PEER_OPT_ADDRS			(1ull << 2ull)
	{ PEER_OPT_ADDRS,		"addresses",	required_argument,	NULL,	'A' },

	#define PEER_OPT_BIND_DEV		(1ull << 3ull)
	{ PEER_OPT_BIND_DEV,		"bind-dev",	required_argument,	NULL,	'b' },

	#define PEER_OPT_BIND_IP		(1ull << 4ull)
	{ PEER_OPT_BIND_IP,		"bind-ip",	required_argument,	NULL,	'B' },

	#define PEER_OPT_BIND_GATEWAY		(1ull << 5ull)
	{ PEER_OPT_BIND_GATEWAY,	"bind-gateway",	required_argument,	NULL,	'g' },

	#define PEER_OPT_FORCE			(1ull << 62ull)
	{ PEER_OPT_FORCE,		"force",	no_argument,		NULL,	'f' },

	#define PEER_OPT_UP			(1ull << 63ull)
	{ PEER_OPT_UP,			"up",		no_argument,		NULL,	'u' },

	{ 0, NULL, 0, NULL, 0 }
};

struct wgm_peer_arg {
	const char		*dev;
	const char		*private_key;
	struct wgm_array_str	addrs;
	const char		*bind_dev;
	const char		*bind_ip;
	const char		*bind_gateway;
	bool			force;
	bool			up;
};

void wgm_cmd_peer_show_usage(const char *app, int show_cmds)
{
	printf("Usage: %s peer [add|update|del|show|list] <arguments>\n", app);
	if (show_cmds) {
		printf("\nCommands:\n");
		printf("  add    <options>      Add a new peer\n");
		printf("  update <options>      Update an existing peer\n");
		printf("  del    <options>      Delete a peer\n");
		printf("  show   <options>      Show a peer\n");
		printf("  list   <options>      List all peers\n");
		printf("\n  --dev option is mandatory for all commands\n");
	}

	printf("\nOptions:\n");
	printf("  -d, --dev <dev>              Interface name\n");
	printf("  -k, --private-key <key>      Private key\n");
	printf("  -A, --addresses <addr>       Peer addresses\n");
	printf("  -b, --bind-dev <dev>         Bind device\n");
	printf("  -B, --bind-ip <ip>           Bind IP address\n");
	printf("  -g, --bind-gateway <ip>      Bind gateway\n");
	printf("  -f, --force                  Force operation\n");
	printf("  -u, --up                     Bring up the peer\n");
	printf("\n");
}

static void wgm_peer_arg_free(struct wgm_peer_arg *arg)
{
	wgm_array_str_free(&arg->addrs);
	memset(arg, 0, sizeof(*arg));
}

static int parse_args(int argc, char *argv[], struct wgm_peer_arg *arg,
		      uint64_t *out_args_p)
{
	struct option *long_opt;
	uint64_t out_args = 0;
	char *short_opt;
	int ret;

	ret = wgm_create_getopt_long_args(&long_opt, &short_opt, options, ARRAY_SIZE(options));
	if (ret)
		return ret;

	memset(arg, 0, sizeof(*arg));
	while (1) {
		int c = getopt_long(argc, argv, short_opt, long_opt, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'd':
			arg->dev = optarg;
			out_args |= PEER_OPT_DEV;
			break;

		case 'k':
			arg->private_key = optarg;
			out_args |= PEER_OPT_PRIVATE_KEY;
			break;

		case 'A':
			wgm_array_str_free(&arg->addrs);
			ret = wgm_array_str_from_csv(&arg->addrs, optarg);
			if (ret)
				goto out;

			out_args |= PEER_OPT_ADDRS;
			break;

		case 'b':
			arg->bind_dev = optarg;
			out_args |= PEER_OPT_BIND_DEV;
			break;

		case 'B':
			arg->bind_ip = optarg;
			out_args |= PEER_OPT_BIND_IP;
			break;

		case 'g':
			arg->bind_gateway = optarg;
			out_args |= PEER_OPT_BIND_GATEWAY;
			break;

		case 'f':
			arg->force = true;
			out_args |= PEER_OPT_FORCE;
			break;

		case 'u':
			arg->up = true;
			out_args |= PEER_OPT_UP;
			break;

		default:
			wgm_cmd_peer_show_usage(argv[0], 0);
			ret = -EINVAL;
			goto out;
		}
	}

out:
	wgm_free_getopt_long_args(long_opt, short_opt);
	*out_args_p = out_args;
	return ret;
}

static int wgm_cmd_peer_run(const char *app, struct wgm_ctx *ctx, const char *cmd,
			    struct wgm_peer_arg *arg, uint64_t out_args)
{
}

int wgm_cmd_peer(int argc, char *argv[], struct wgm_ctx *ctx)
{
	struct wgm_peer_arg arg;
	const char *cmd, *app;
	uint64_t out_args = 0;
	int ret;

	if (argc < 3) {
		wgm_cmd_peer_show_usage(argv[0], 1);
		return -EINVAL;
	}

	app = argv[0];
	cmd = argv[2];
	ret = parse_args(argc, argv, &arg, &out_args);
	if (ret)
		return ret;

	ret = wgm_cmd_peer_run(app, ctx, cmd, &arg, out_args);
	wgm_peer_arg_free(&arg);
	return 0;
}

int wgm_peer_to_json(const struct wgm_peer *peer, json_object **ret)
{
	json_object *tmp;
	int err = 0;

	tmp = json_object_new_object();
	if (!tmp)
		return -ENOMEM;

	err |= wgm_json_obj_set_str(tmp, "public_key", peer->public_key);
	err |= wgm_json_obj_set_str_array(tmp, "addresses", &peer->addresses);
	if (err) {
		json_object_put(tmp);
		return -ENOMEM;
	}

	*ret = tmp;
	return 0;
}

int wgm_peer_from_json(struct wgm_peer *peer, const json_object *obj)
{
	int err = 0;

	memset(peer, 0, sizeof(*peer));
	err |= wgm_json_obj_kcp_str(obj, "public_key", peer->public_key, sizeof(peer->public_key));
	err |= wgm_json_obj_kcp_str_array(obj, "addresses", &peer->addresses);
	if (err) {
		wgm_peer_free(peer);
		return -EINVAL;
	}

	return 0;
}

int wgm_peer_copy(struct wgm_peer *dst, const struct wgm_peer *src)
{
	struct wgm_peer tmp;

	tmp = *src;

	if (wgm_array_str_copy(&tmp.addresses, &src->addresses))
		return -ENOMEM;

	*dst = tmp;
	return 0;
}

void wgm_peer_move(struct wgm_peer *dst, struct wgm_peer *src)
{
	*dst = *src;
	wgm_array_str_move(&dst->addresses, &src->addresses);
	wgm_peer_free(src);
}

void wgm_peer_free(struct wgm_peer *peer)
{
	wgm_array_str_free(&peer->addresses);
	memset(peer, 0, sizeof(*peer));
}

int wgm_array_peer_add(struct wgm_array_peer *arr, const struct wgm_peer *peer)
{
	struct wgm_peer *new_arr;
	size_t new_len;

	new_len = arr->len + 1;
	new_arr = realloc(arr->arr, new_len * sizeof(*new_arr));
	if (!new_arr)
		return -ENOMEM;

	arr->arr = new_arr;
	if (wgm_peer_copy(&arr->arr[arr->len], peer))
		return -ENOMEM;

	arr->len = new_len;
	return 0;
}

int wgm_array_peer_del(struct wgm_array_peer *arr, size_t idx)
{
	if (idx >= arr->len)
		return -EINVAL;

	memmove(&arr->arr[idx], &arr->arr[idx + 1], (arr->len - idx - 1) * sizeof(*arr->arr));
	arr->len--;

	return 0;
}

int wgm_array_peer_find(struct wgm_array_peer *arr, const char *pub_key, size_t *idx)
{
	size_t i;

	for (i = 0; i < arr->len; i++) {
		if (strcmp(arr->arr[i].public_key, pub_key) == 0) {
			*idx = i;
			return 0;
		}
	}

	return -ENOENT;
}

int wgm_array_peer_add_unique(struct wgm_array_peer *arr, const struct wgm_peer *peer)
{
	size_t idx;
	int ret;

	ret = wgm_array_peer_find(arr, peer->public_key, &idx);
	if (ret == 0)
		return -EEXIST;

	return wgm_array_peer_add(arr, peer);
}

int wgm_array_peer_add_mv(struct wgm_array_peer *arr, struct wgm_peer *peer)
{
	int ret;

	ret = wgm_array_peer_add(arr, peer);
	if (ret)
		wgm_peer_free(peer);

	return ret;
}

int wgm_array_peer_add_mv_unique(struct wgm_array_peer *arr, struct wgm_peer *peer)
{
	int ret;

	ret = wgm_array_peer_add_unique(arr, peer);
	if (ret)
		wgm_peer_free(peer);

	return ret;
}

int wgm_array_peer_copy(struct wgm_array_peer *dst, const struct wgm_array_peer *src)
{
	struct wgm_peer *new_arr;
	size_t i;

	new_arr = malloc(src->len * sizeof(*new_arr));
	if (!new_arr)
		return -ENOMEM;

	for (i = 0; i < src->len; i++) {
		if (wgm_peer_copy(&new_arr[i], &src->arr[i])) {
			while (i--)
				wgm_peer_free(&new_arr[i]);
			free(new_arr);
			return -ENOMEM;
		}
	}

	dst->arr = new_arr;
	dst->len = src->len;
	return 0;
}

void wgm_array_peer_move(struct wgm_array_peer *dst, struct wgm_array_peer *src)
{
	*dst = *src;
	memset(src, 0, sizeof(*src));
}

void wgm_array_peer_free(struct wgm_array_peer *peer_array)
{
	size_t i;

	for (i = 0; i < peer_array->len; i++)
		wgm_peer_free(&peer_array->arr[i]);

	free(peer_array->arr);
	memset(peer_array, 0, sizeof(*peer_array));
}

int wgm_array_peer_to_json(const struct wgm_array_peer *arr, json_object **obj)
{
	json_object *tmp;
	size_t i;
	int err = 0;

	*obj = json_object_new_array();
	if (!*obj)
		return -ENOMEM;

	for (i = 0; i < arr->len; i++) {
		err = wgm_peer_to_json(&arr->arr[i], &tmp);
		if (err)
			goto out_err;

		err = json_object_array_add(*obj, tmp);
		if (err)
			goto out_err;
	}

	return 0;

out_err:
	json_object_put(*obj);
	return -ENOMEM;
}

int wgm_array_peer_from_json(struct wgm_array_peer *arr, const json_object *obj)
{
	return 0;
}
