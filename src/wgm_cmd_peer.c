// SPDX-License-Identifier: GPL-2.0-only

#include <string.h>
#include <assert.h>

#include "wgm_cmd_peer.h"
#include "wgm_cmd_iface.h"
#include "wgm.h"

static const struct wgm_opt options[] = {
	#define PEER_OPT_DEV			(1ull << 0ull)
	{ PEER_OPT_DEV,			"dev",		required_argument,	NULL,	'd' },

	#define PEER_OPT_PUBLIC_KEY		(1ull << 1ull)
	{ PEER_OPT_PUBLIC_KEY,		"public-key",	required_argument,	NULL,	'k' },

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
	const char		*public_key;
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
	printf("  -k, --public-key <key>       Public key\n");
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
			arg->public_key = optarg;
			out_args |= PEER_OPT_PUBLIC_KEY;
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

static bool validate_args(const char *app, struct wgm_peer_arg *arg,
			  uint64_t arg_bits, uint64_t required_bits,
			  uint64_t allowed_bits)

{
	size_t i;

	assert((required_bits & allowed_bits) == required_bits);
	for (i = 0; i < ARRAY_SIZE(options); i++) {
		uint64_t id = options[i].id;

		if (!id)
			continue;

		if ((id & required_bits) && !(id & arg_bits)) {
			wgm_err_elog_add("Error: Missing required option: --%s\n", options[i].name);
			wgm_cmd_peer_show_usage(app, 1);
			return false;
		}

		if (!(id & allowed_bits) && (id & arg_bits)) {
			wgm_err_elog_add("Option --%s is not allowed for this command.\n", options[i].name);
			wgm_cmd_peer_show_usage(app, 1);
			return false;
		}
	}

	return true;
}

static int wg_apply_arg_to_peer(struct wgm_peer *peer, struct wgm_peer_arg *arg,
				uint64_t arg_bits)
{
	if (arg_bits & PEER_OPT_PUBLIC_KEY) {
		if (strlen(arg->public_key) > sizeof(peer->public_key)) {
			wgm_err_elog_add("Error: Private key is too long\n");
			return -EINVAL;
		}

		strncpyl(peer->public_key, arg->public_key, sizeof(peer->public_key));
	}

	if (arg_bits & PEER_OPT_ADDRS)
		wgm_array_str_move(&peer->addresses, &arg->addrs);

	if (arg_bits & PEER_OPT_BIND_DEV)
		strncpyl(peer->bind_dev, arg->bind_dev, sizeof(peer->bind_dev));

	if (arg_bits & PEER_OPT_BIND_IP)
		strncpyl(peer->bind_ip, arg->bind_ip, sizeof(peer->bind_ip));

	if (arg_bits & PEER_OPT_BIND_GATEWAY)
		strncpyl(peer->bind_gateway, arg->bind_gateway, sizeof(peer->bind_gateway));

	return 0;
}

static int do_peer_add_or_update(struct wgm_ctx *ctx, struct wgm_peer_arg *arg,
				 uint64_t arg_bits, bool is_update)
{
	struct wgm_iface_hdl hdl;
	struct wgm_iface iface;
	size_t idx;
	int ret;

	memset(&iface, 0, sizeof(iface));
	memset(&hdl, 0, sizeof(hdl));

	hdl.ctx = ctx;
	ret = wgm_iface_hdl_open_and_load(&hdl, arg->dev, false, &iface);
	if (ret) {
		wgm_err_elog_add("Error: Failed to load interface: %s\n", arg->dev);
		return ret;
	}

	/*
	 * Try to find the peer in the interface, if it exists, update will
	 * be performed if --force is set, otherwise return -EEXIST.
	 */
	ret = wgm_array_peer_find(&iface.peers, arg->public_key, &idx);
	if (is_update) {
		if (ret == -ENOENT) {
			wgm_err_elog_add("Error: Peer %s does not exist, use command 'add' to add a new peer\n", arg->public_key);
			goto out;
		}

		if (ret) {
			wgm_err_elog_add("Error: Failed to find peer %s: %s\n", arg->public_key, strerror(-ret));
			goto out;
		}

		ret = wg_apply_arg_to_peer(&iface.peers.arr[idx], arg, arg_bits);
		if (ret) {
			wgm_err_elog_add("Error: Failed to update peer %s: %s\n", arg->public_key, strerror(-ret));
			goto out;
		}
	} else {
		if (ret) {
			struct wgm_peer peer;

			if (ret != -ENOENT) {
				wgm_err_elog_add("Error: Failed to find peer %s: %s (expected -ENOENT return, but %d returned)\n", arg->public_key, strerror(-ret), ret);
				goto out;
			}

			memset(&peer, 0, sizeof(peer));
			ret = wg_apply_arg_to_peer(&peer, arg, arg_bits);
			if (ret) {
				wgm_err_elog_add("Error: Failed to apply arguments to peer: %s\n", strerror(-ret));
				goto out;
			}

			ret = wgm_array_peer_add_mv_unique(&iface.peers, &peer);
			if (ret) {
				wgm_err_elog_add("Error: Failed to add peer: %s\n", strerror(-ret));
				wgm_peer_free(&peer);
				goto out;
			}
		} else {
			if (!arg->force) {
				wgm_err_elog_add("Error: Peer %s already exists, use --force to force update\n", arg->public_key);
				ret = -EEXIST;
				goto out;
			}

			ret = wg_apply_arg_to_peer(&iface.peers.arr[idx], arg, arg_bits);
			if (ret) {
				wgm_err_elog_add("Error: Failed to update peer %s: %s\n", arg->public_key, strerror(-ret));
				goto out;
			}
		}
	}

	ret = wgm_iface_hdl_store(&hdl, &iface);
	if (ret)
		wgm_err_elog_add("Error: Failed to store interface: %s\n", arg->dev);

out:
	wgm_iface_hdl_close(&hdl);
	wgm_iface_free(&iface);
	return ret;
}

static int wgm_cmd_peer_add(const char *app, struct wgm_ctx *ctx,
			    struct wgm_peer_arg *arg, uint64_t arg_bits)
{
	static const uint64_t required_bits = PEER_OPT_DEV |
					      PEER_OPT_PUBLIC_KEY |
					      PEER_OPT_ADDRS;

	static const uint64_t allowed_bits = required_bits |
					       PEER_OPT_BIND_DEV |
					       PEER_OPT_BIND_IP |
					       PEER_OPT_BIND_GATEWAY |
					       PEER_OPT_FORCE |
					       PEER_OPT_UP;

	static const bool is_update = false;

	if (!validate_args(app, arg, arg_bits, required_bits, allowed_bits))
		return -EINVAL;

	return do_peer_add_or_update(ctx, arg, arg_bits, is_update);
}

static int wgm_cmd_peer_update(const char *app, struct wgm_ctx *ctx,
			       struct wgm_peer_arg *arg, uint64_t arg_bits)
{
	static const uint64_t required_bits = PEER_OPT_DEV |
					      PEER_OPT_PUBLIC_KEY;

	static const uint64_t allowed_bits = required_bits |
					       PEER_OPT_ADDRS |
					       PEER_OPT_BIND_DEV |
					       PEER_OPT_BIND_IP |
					       PEER_OPT_BIND_GATEWAY |
					       PEER_OPT_FORCE |
					       PEER_OPT_UP;

	static const bool is_update = true;

	if (!validate_args(app, arg, arg_bits, required_bits, allowed_bits))
		return -EINVAL;

	return do_peer_add_or_update(ctx, arg, arg_bits, is_update);
}

static int wgm_cmd_peer_run(const char *app, struct wgm_ctx *ctx, const char *cmd,
			    struct wgm_peer_arg *arg, uint64_t out_args)
{
	if (!strcmp(cmd, "add"))
		return wgm_cmd_peer_add(app, ctx, arg, out_args);

	if (!strcmp(cmd, "update"))
		return wgm_cmd_peer_update(app, ctx, arg, out_args);

	wgm_err_elog_add("Error: Unknown command: '%s'\n", cmd);
	wgm_cmd_peer_show_usage(app, 1);
	return -EINVAL;
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
	if (!ret)
		wgm_peer_free(peer);

	return ret;
}

int wgm_array_peer_add_mv_unique(struct wgm_array_peer *arr, struct wgm_peer *peer)
{
	int ret;

	ret = wgm_array_peer_add_unique(arr, peer);
	if (!ret)
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
	json_object *tmp;
	size_t i, len;
	int ret;

	if (!json_object_is_type(obj, json_type_array))
		return -EINVAL;

	memset(arr, 0, sizeof(*arr));
	len = json_object_array_length(obj);
	for (i = 0; i < len; i++) {
		struct wgm_peer p;

		tmp = json_object_array_get_idx(obj, i);
		if (!json_object_is_type(tmp, json_type_object)) {
			ret = -EINVAL;
			break;
		}

		ret = wgm_peer_from_json(&p, tmp);
		if (ret)
			break;

		ret = wgm_array_peer_add_mv(arr, &p);
		if (ret) {
			wgm_peer_free(&p);
			break;
		}
	}

	if (ret)
		wgm_array_peer_free(arr);

	return 0;
}
