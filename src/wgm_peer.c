// SPDX-License-Identifier: GPL-2.0-only

#include "wgm.h"
#include "wgm_peer.h"
#include "wgm_iface.h"

struct wgm_peer_arg {
	char			ifname[IFNAMSIZ];
	char			public_key[256];
	char			endpoint[128];
	char			bind_ip[16];
	struct wgm_str_array	allowed_ips;
	bool			force;
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
	{ PEER_ARG_HELP,	"help",		no_argument,		NULL,	'h' },

	#define PEER_ARG_FORCE		(1ull << 6)
	{ PEER_ARG_FORCE,	"force",	no_argument,		NULL,	'f' },

	{ 0, NULL, 0, NULL, 0 }
};

static void wgm_peer_show_usage(void)
{
	show_usage_peer(NULL, false);
}

static int wgm_peer_opt_get_dev(char *ifname, size_t iflen, const char *dev)
{
	return wgm_iface_opt_get_dev(ifname, iflen, dev);
}

static int wgm_peer_opt_get_public_key(char *private_key, size_t keylen,
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

static int wgm_peer_opt_get_endpoint(char *endpoint, size_t endlen,
				     const char *end)
{
	size_t i;

	i = strlen(end);
	if (i >= endlen) {
		wgm_log_err("Error: Endpoint is too long, max %zu characters\n",
			    endlen - 1);
		return -EINVAL;
	}

	if (!i) {
		wgm_log_err("Error: Endpoint cannot be empty\n");
		return -EINVAL;
	}

	strncpyl(endpoint, end, endlen);
	return 0;
}

static int wgm_peer_opt_get_bind_ip(char *bind_ip, size_t iplen, const char *ip)
{
	size_t i;

	i = strlen(ip);
	if (i >= iplen) {
		wgm_log_err("Error: Bind IP is too long, max %zu characters\n",
			    iplen - 1);
		return -EINVAL;
	}

	if (!i) {
		wgm_log_err("Error: Bind IP cannot be empty\n");
		return -EINVAL;
	}

	strncpyl(bind_ip, ip, iplen);
	return 0;
}

static int wgm_peer_opt_get_allowed_ips(struct wgm_str_array *allowed_ips,
					const char *ips)
{
	return wgm_parse_csv(allowed_ips, ips);
}

static int wgm_peer_getopt(int argc, char *argv[], struct wgm_peer_arg *arg,
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
			if (wgm_peer_opt_get_dev(arg->ifname, sizeof(arg->ifname), optarg))
				return -EINVAL;
			out_args |= PEER_ARG_DEV;
			break;
		case 'p':
			if (wgm_peer_opt_get_public_key(arg->public_key, sizeof(arg->public_key), optarg))
				return -EINVAL;
			out_args |= PEER_ARG_PUBLIC_KEY;
			break;
		case 'e':
			if (wgm_peer_opt_get_endpoint(arg->endpoint, sizeof(arg->endpoint), optarg))
				return -EINVAL;
			out_args |= PEER_ARG_ENDPOINT;
			break;
		case 'b':
			if (wgm_peer_opt_get_bind_ip(arg->bind_ip, sizeof(arg->bind_ip), optarg))
				return -EINVAL;
			out_args |= PEER_ARG_BIND_IP;
			break;
		case 'a':
			if (wgm_peer_opt_get_allowed_ips(&arg->allowed_ips, optarg))
				return -EINVAL;
			out_args |= PEER_ARG_ALLOWED_IPS;
			break;
		case 'h':
			wgm_peer_show_usage();
			ret = -1;
			goto out;
		case 'f':
			arg->force = true;
			out_args |= PEER_ARG_FORCE;
			break;
		case '?':
			ret = -EINVAL;
			goto out;
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
			wgm_peer_show_usage();
			ret = -EINVAL;
			goto out;
		}

		if ((cid & required_args) && !(cid & out_args)) {
			wgm_log_err("Error: Option '--%s' is required\n\n", name);
			wgm_peer_show_usage();
			ret = -EINVAL;
			goto out;
		}
	}

	*out_args_p = out_args;

out:
	wgm_free_getopt_long_args(long_opt, short_opt);
	return ret;
}

static void wgm_peer_arg_free(struct wgm_peer_arg *arg)
{
	wgm_str_array_free(&arg->allowed_ips);
	memset(arg, 0, sizeof(*arg));
}

static void apply_wgm_arg(struct wgm_peer *peer, struct wgm_peer_arg *arg,
			  uint64_t out_args)
{
	if (out_args & PEER_ARG_PUBLIC_KEY)
		memcpy(peer->public_key, arg->public_key, sizeof(peer->public_key));

	if (out_args & PEER_ARG_BIND_IP)
		memcpy(peer->bind_ip, arg->bind_ip, sizeof(peer->bind_ip));

	if (out_args & PEER_ARG_ALLOWED_IPS)
		wgm_str_array_move(&peer->allowed_ips, &arg->allowed_ips);
}


int wgm_peer_cmd_add(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t required_args = PEER_ARG_DEV | PEER_ARG_PUBLIC_KEY |
					      PEER_ARG_ALLOWED_IPS;
	static const uint64_t allowed_args = required_args | PEER_ARG_ENDPOINT |
					     PEER_ARG_BIND_IP | PEER_ARG_FORCE |
					     PEER_ARG_HELP;

	struct wgm_peer_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	struct wgm_peer peer;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));
	memset(&peer, 0, sizeof(peer));

	ret = wgm_peer_getopt(argc, argv, &arg, allowed_args, required_args, &out_args);
	if (ret)
		goto out;

	ret = wgm_iface_load(&iface, ctx, arg.ifname);
	if (ret) {
		wgm_log_err("Error: Failed to load interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	apply_wgm_arg(&peer, &arg, out_args);
	ret = wgm_iface_add_peer(&iface, &peer, arg.force);
	if (ret) {
		wgm_log_err("Error: Failed to add peer to interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	ret = wgm_iface_save(&iface, ctx);
	if (ret) {
		wgm_log_err("Error: Failed to save interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	wgm_iface_dump_json(&iface);
out:
	wgm_peer_arg_free(&arg);
	wgm_iface_free(&iface);
	wgm_peer_free(&peer);
	return ret;
}

int wgm_peer_cmd_del(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t required_args = PEER_ARG_DEV | PEER_ARG_PUBLIC_KEY;
	static const uint64_t allowed_args = required_args | PEER_ARG_FORCE | PEER_ARG_HELP;

	struct wgm_peer_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	struct wgm_peer peer;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));
	memset(&peer, 0, sizeof(peer));

	ret = wgm_peer_getopt(argc, argv, &arg, allowed_args, required_args, &out_args);
	if (ret)
		goto out;

	ret = wgm_iface_load(&iface, ctx, arg.ifname);
	if (ret) {
		wgm_log_err("Error: Failed to load interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	apply_wgm_arg(&peer, &arg, out_args);
	ret = wgm_iface_del_peer_by_pubkey(&iface, peer.public_key);
	if (ret) {
		wgm_log_err("Error: Failed to delete peer from interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	ret = wgm_iface_save(&iface, ctx);
	if (ret) {
		wgm_log_err("Error: Failed to save interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	wgm_iface_dump_json(&iface);
	ret = 0;
out:
	wgm_peer_arg_free(&arg);
	wgm_iface_free(&iface);
	wgm_peer_free(&peer);
	return ret;
}

static void wgm_peer_dump_json(const struct wgm_peer *peer)
{
	json_object *jpeer;
	int ret;

	ret = wgm_peer_to_json(&jpeer, peer);
	if (ret) {
		wgm_log_err("Error: Failed to convert peer to JSON: %s\n", strerror(-ret));
		return;
	}

	printf("%s\n", json_object_to_json_string_ext(jpeer, WGM_JSON_FLAGS));
	json_object_put(jpeer);
}

int wgm_peer_cmd_show(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t required_args = PEER_ARG_DEV | PEER_ARG_PUBLIC_KEY;
	static const uint64_t allowed_args = required_args | PEER_ARG_HELP;

	struct wgm_peer *peer_p;
	struct wgm_peer_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));

	ret = wgm_peer_getopt(argc, argv, &arg, allowed_args, required_args, &out_args);
	if (ret)
		goto out;

	ret = wgm_iface_load(&iface, ctx, arg.ifname);
	if (ret) {
		wgm_log_err("Error: Failed to load interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	ret = wgm_iface_get_peer_by_pubkey(&iface, arg.public_key, &peer_p);
	if (ret) {
		wgm_log_err("Error: Failed to get peer from interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	wgm_peer_dump_json(peer_p);
	ret = 0;

out:
	wgm_peer_arg_free(&arg);
	wgm_iface_free(&iface);
	return ret;
}

int wgm_peer_cmd_update(int argc, char *argv[], struct wgm_ctx *ctx)
{
	const uint64_t required_args = PEER_ARG_DEV | PEER_ARG_PUBLIC_KEY;
	const uint64_t allowed_args = required_args | PEER_ARG_ENDPOINT |
				      PEER_ARG_BIND_IP | PEER_ARG_ALLOWED_IPS |
				      PEER_ARG_FORCE | PEER_ARG_HELP;

	struct wgm_peer *peer_p;
	struct wgm_peer_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));

	ret = wgm_peer_getopt(argc, argv, &arg, allowed_args, required_args, &out_args);
	if (ret)
		goto out;

	ret = wgm_iface_load(&iface, ctx, arg.ifname);
	if (ret) {
		wgm_log_err("Error: Failed to load interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	ret = wgm_iface_get_peer_by_pubkey(&iface, arg.public_key, &peer_p);
	if (ret) {
		wgm_log_err("Error: Failed to get peer from interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	apply_wgm_arg(peer_p, &arg, out_args);
	ret = wgm_iface_save(&iface, ctx);
	if (ret) {
		wgm_log_err("Error: Failed to save interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	wgm_iface_dump_json(&iface);
	ret = 0;

out:
	wgm_peer_arg_free(&arg);
	wgm_iface_free(&iface);
	return ret;
}

int wgm_peer_cmd_list(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t required_args = PEER_ARG_DEV;
	static const uint64_t allowed_args = required_args | PEER_ARG_HELP;

	struct wgm_peer_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));

	ret = wgm_peer_getopt(argc, argv, &arg, allowed_args, required_args, &out_args);
	if (ret)
		goto out;

	ret = wgm_iface_load(&iface, ctx, arg.ifname);
	if (ret) {
		wgm_log_err("Error: Failed to load interface '%s': %s\n", arg.ifname, strerror(-ret));
		goto out;
	}

	wgm_iface_peer_array_dump_json(&iface.peers);
	ret = 0;

out:
	wgm_peer_arg_free(&arg);
	wgm_iface_free(&iface);
	return ret;
}

int wgm_peer_copy(struct wgm_peer *dst, const struct wgm_peer *src)
{
	memcpy(dst->public_key, src->public_key, sizeof(dst->public_key));
	memcpy(dst->bind_ip, src->bind_ip, sizeof(dst->bind_ip));
	return wgm_str_array_copy(&dst->allowed_ips, &src->allowed_ips);
}

void wgm_peer_move(struct wgm_peer *dst, struct wgm_peer *src)
{
	memcpy(dst->public_key, src->public_key, sizeof(dst->public_key));
	memcpy(dst->bind_ip, src->bind_ip, sizeof(dst->bind_ip));
	wgm_str_array_move(&dst->allowed_ips, &src->allowed_ips);
	memset(src, 0, sizeof(*src));
}

void wgm_peer_free(struct wgm_peer *peer)
{
	wgm_str_array_free(&peer->allowed_ips);
	memset(peer, 0, sizeof(*peer));
}

int wgm_peer_to_json(json_object **jobj, const struct wgm_peer *peer)
{
	json_object *jpeer, *jallowed_ips;
	int ret;

	jpeer = json_object_new_object();
	if (!jpeer)
		return -ENOMEM;

	ret = json_object_object_add(jpeer, "public_key",
				     json_object_new_string(peer->public_key));
	if (ret)
		goto out;

	ret = json_object_object_add(jpeer, "bind_ip",
				     json_object_new_string(peer->bind_ip));
	if (ret)
		goto out;

	ret = wgm_str_array_to_json(&jallowed_ips, &peer->allowed_ips);
	if (ret)
		goto out;

	ret = json_object_object_add(jpeer, "allowed_ips", jallowed_ips);
	if (ret)
		goto out;

	*jobj = jpeer;
	return 0;

out:
	json_object_put(jpeer);
	return ret;
}

int wgm_peer_from_json(struct wgm_peer *peer, const json_object *jobj)
{
	json_object *tmp;
	int ret;

	ret = json_object_object_get_ex(jobj, "public_key", &tmp);
	if (!ret)
		return -EINVAL;

	strncpyl(peer->public_key, json_object_get_string(tmp), sizeof(peer->public_key));

	ret = json_object_object_get_ex(jobj, "bind_ip", &tmp);
	if (!ret)
		return -EINVAL;

	strncpyl(peer->bind_ip, json_object_get_string(tmp), sizeof(peer->bind_ip));

	ret = json_object_object_get_ex(jobj, "allowed_ips", &tmp);
	if (!ret)
		return -EINVAL;

	memset(&peer->allowed_ips, 0, sizeof(peer->allowed_ips));
	return wgm_str_array_from_json(&peer->allowed_ips, tmp);
}
