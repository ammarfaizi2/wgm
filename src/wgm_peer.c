// SPDX-License-Identifier: GPL-2.0-only
#include "wgm_iface.h"
#include "wgm_peer.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json.h>

enum {
	WGM_PEER_ARG_HAS_IFNAME		= (1ull << 0ull),
	WGM_PEER_ARG_HAS_PUBLIC_KEY	= (1ull << 1ull),
	WGM_PEER_ARG_HAS_ALLOWED_IPS	= (1ull << 2ull),
};

static const char wgm_iface_opt_short[] = "i:p:a:b:hf";
static const struct option wgm_iface_opt_long[] = {
	{"ifname",		required_argument,	NULL,	'i'},
	{"public-key",		required_argument,	NULL,	'p'},
	{"allowed-ips",		required_argument,	NULL,	'a'},
	{"bind-ip",		required_argument,	NULL,	'b'},
	{"help",		no_argument,		NULL,	'h'},
	{"force",		no_argument,		NULL,	'f'},
	{NULL,			0,			NULL,	0},
};

static void wgm_iface_add_help(const char *app)
{
	printf("Usage: %s peer [options]\n", app);
	printf("Options:\n");
	printf("  -i, --ifname=IFNAME\t\tInterface name\n");
	printf("  -p, --public-key=KEY\t\tPublic key\n");
	printf("  -a, --allowed-ips=IPS\t\tAllowed IPs\n");
	printf("  -b, --bind-ip=IP\t\tBind IP\n");
	printf("  -h, --help\t\t\tDisplay this help\n");
	printf("  -f, --force\t\t\tForce update\n");
}

static int wgm_peer_parse_args(int argc, char *argv[], struct wgm_peer_arg *arg)
{
	uint64_t flags = 0;
	char tmp_ip[16];
	int c;

	arg->force = false;

	while (1) {
		c = getopt_long(argc, argv, wgm_iface_opt_short, wgm_iface_opt_long, NULL);
		if (c < 0)
			break;

		switch (c) {
		case 'i':
			if (wgm_parse_ifname(optarg, arg->ifname))
				return -EINVAL;
			flags |= WGM_PEER_ARG_HAS_IFNAME;
			break;
		case 'p':
			if (wgm_parse_key(optarg, arg->public_key, sizeof(arg->public_key)) < 0)
				return -EINVAL;
			flags |= WGM_PEER_ARG_HAS_PUBLIC_KEY;
			break;
		case 'a':
			arg->allowed_ips = strdup(optarg);
			if (!arg->allowed_ips)
				return -ENOMEM;
			flags |= WGM_PEER_ARG_HAS_ALLOWED_IPS;
			break;
		case 'b':
			if (inet_pton(AF_INET, optarg, tmp_ip) <= 0) {
				if (inet_pton(AF_INET6, optarg, tmp_ip) <= 0) {
					printf("Error: invalid IP address: %s\n", optarg);
					return -EINVAL;
				}
			}
			strncpyl(arg->bind_ip, optarg, sizeof(arg->bind_ip));
			break;
		case 'f':
			arg->force = true;
			break;
		case 'h':
			wgm_iface_add_help(argv[0]);
			return -1;
		default:
			return -EINVAL;
		}
	}

	if (!(flags & WGM_PEER_ARG_HAS_IFNAME)) {
		printf("Error: wgm_peer_parse_args: missing interface name argument\n\n");
		goto err;
	}

	if (!(flags & WGM_PEER_ARG_HAS_PUBLIC_KEY)) {
		printf("Error: wgm_peer_parse_args: missing public key argument\n\n");
		goto err;
	}

	if (!(flags & WGM_PEER_ARG_HAS_ALLOWED_IPS)) {
		printf("Error: wgm_peer_parse_args: missing allowed IPs argument\n\n");
		goto err;
	}

	return 0;

err:
	wgm_iface_add_help(argv[0]);
	return -EINVAL;
}

int wgm_peer_add(int argc, char *argv[], struct wgm_ctx *ctx)
{
	struct wgm_peer_arg arg;
	struct wgm_iface iface;
	struct wgm_peer peer;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));
	memset(&peer, 0, sizeof(peer));

	ret = wgm_peer_parse_args(argc, argv, &arg);
	if (ret < 0)
		return ret;

	ret = wgm_iface_load(ctx, &iface, arg.ifname);
	if (ret < 0) {
		printf("Error: wgm_peer_add: failed to load interface \"%s\": %s\n", arg.ifname, strerror(-ret));
		return ret;
	}

	ret = wgm_parse_csv(&peer.allowed_ips, arg.allowed_ips);
	if (ret < 0) {
		printf("Error: wgm_peer_add: failed to parse allowed IPs\n");
		return ret;
	}

	strncpyl(peer.public_key, arg.public_key, sizeof(peer.public_key));
	strncpyl(peer.bind_ip, arg.bind_ip, sizeof(peer.bind_ip));

	ret = wgm_iface_add_peer(&iface, &peer);
	if (ret < 0) {
		if (ret != -EEXIST) {
			printf("Error: wgm_peer_add: failed to add peer (to iface)\n");
			return ret;
		}

		if (!arg.force) {
			printf("Error: peer already exists, use --force to force update\n");
			return -EEXIST;
		}

		ret = wgm_iface_update_peer(&iface, &peer);
		if (ret < 0) {
			printf("Error: wgm_peer_add: failed to update peer (in iface)\n");
			return ret;
		}
	}

	wgm_iface_dump_json(&iface);
	ret = wgm_iface_save(ctx, &iface);
	wgm_iface_free(&iface);
	return ret;
}

int wgm_peer_update(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}

void wgm_peer_free(struct wgm_peer *peer)
{
	wgm_str_array_free(&peer->allowed_ips);
	memset(peer, 0, sizeof(*peer));
}

void wgm_peer_dump(const struct wgm_peer *peer)
{
	printf("      Public key: %s\n", peer->public_key);
	printf("      Bind IP: %s\n", peer->bind_ip);
	printf("      Allowed IPs:\n");
	wgm_str_array_dump(&peer->allowed_ips);
}

int wgm_peer_to_json(const struct wgm_peer *peer, json_object **obj)
{
	json_object *jobj, *tmp;
	int ret;

	jobj = json_object_new_object();
	if (!jobj)
		return -ENOMEM;

	tmp = json_object_new_string(peer->public_key);
	if (!tmp) {
		ret = -ENOMEM;
		goto out;
	}

	ret = json_object_object_add(jobj, "public_key", tmp);
	if (ret < 0) {
		json_object_put(tmp);
		goto out;
	}

	tmp = json_object_new_string(peer->bind_ip);
	if (!tmp) {
		ret = -ENOMEM;
		goto out;
	}

	ret = json_object_object_add(jobj, "bind_ip", tmp);
	if (ret < 0) {
		json_object_put(tmp);
		goto out;
	}

	ret = wgm_str_array_to_json(&peer->allowed_ips, &tmp);
	if (ret < 0)
		goto out;

	ret = json_object_object_add(jobj, "allowed_ips", tmp);
	if (ret < 0) {
		json_object_put(tmp);
		goto out;
	}

	*obj = jobj;
	return 0;

out:
	json_object_put(jobj);
	return ret;
}

int wgm_json_to_peer(struct wgm_peer *peer, const json_object *obj)
{
	json_object *tmp;
	const char *tstr;

	if (!json_object_object_get_ex(obj, "public_key", &tmp)) {
		printf("Error: wgm_json_to_peer: missing public key\n");
		return -EINVAL;
	}

	if (!tmp || !json_object_is_type(tmp, json_type_string)) {
		printf("Error: wgm_json_to_peer: invalid public key\n");
		return -EINVAL;
	}

	tstr = json_object_get_string(tmp);
	if (!tstr || strlen(tstr) >= sizeof(peer->public_key)) {
		printf("Error: wgm_json_to_peer: invalid public key\n");
		return -EINVAL;
	}

	strncpy(peer->public_key, tstr, sizeof(peer->public_key));

	if (!json_object_object_get_ex(obj, "bind_ip", &tmp)) {
		printf("Error: wgm_json_to_peer: missing bind IP\n");
		return -EINVAL;
	}

	if (!tmp || !json_object_is_type(tmp, json_type_string)) {
		printf("Error: wgm_json_to_peer: invalid bind IP\n");
		return -EINVAL;
	}

	tstr = json_object_get_string(tmp);
	if (!tstr || strlen(tstr) >= sizeof(peer->bind_ip)) {
		printf("Error: wgm_json_to_peer: invalid bind IP\n");
		return -EINVAL;
	}

	strncpy(peer->bind_ip, tstr, sizeof(peer->bind_ip));

	if (!json_object_object_get_ex(obj, "allowed_ips", &tmp)) {
		printf("Error: wgm_json_to_peer: missing allowed IPs\n");
		return -EINVAL;
	}

	if (!tmp || !json_object_is_type(tmp, json_type_array)) {
		printf("Error: wgm_json_to_peer: invalid allowed IPs\n");
		return -EINVAL;
	}

	wgm_str_array_free(&peer->allowed_ips);
	return wgm_json_to_str_array(&peer->allowed_ips, tmp);
}
