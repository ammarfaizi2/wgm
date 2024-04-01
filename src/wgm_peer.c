// SPDX-License-Identifier: GPL-2.0-only
#include "wgm_iface.h"
#include "wgm_peer.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json.h>
#if 0
static const char wgm_iface_opt_short[] = "i:p:a:b:h";
static const struct option wgm_iface_opt_long[] = {
	{"ifname",		required_argument,	NULL,	'i'},
	{"public-key",		required_argument,	NULL,	'p'},
	{"allowed-ips",		required_argument,	NULL,	'a'},
	{"bind-ip",		required_argument,	NULL,	'b'},
	{"help",		no_argument,		NULL,	'h'},
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
}

static int wgm_peer_parse_args(int argc, char *argv[], struct wgm_peer_arg *arg)
{
	char tmp_ip[16];
	int c;

	while (1) {
		c = getopt_long(argc, argv, wgm_iface_opt_short, wgm_iface_opt_long, NULL);
		if (c < 0)
			break;

		switch (c) {
		case 'i':
			if (wgm_parse_ifname(optarg, arg->ifname))
				return -EINVAL;
			break;
		case 'p':
			if (wgm_parse_key(optarg, arg->public_key, sizeof(arg->public_key)) < 0)
				return -EINVAL;
			break;
		case 'a':
			arg->allowed_ips = strdup(optarg);
			if (!arg->allowed_ips)
				return -ENOMEM;
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
		case 'h':
			wgm_iface_add_help(argv[0]);
			return -1;
		default:
			return -EINVAL;
		}
	}

	return 0;
}

static int wgm_parse_allowed_ips(const char *str_ips, char ***allowed_ips, uint16_t *nr_allowed_ips)
{
	size_t i;
	char **arr;
	char *tmp;

	*nr_allowed_ips = 0;
	*allowed_ips = NULL;

	if (!str_ips)
		return 0;

	tmp = strdup(str_ips);
	if (!tmp)
		return -ENOMEM;

	for (i = 0; i < strlen(tmp); i++) {
		if (tmp[i] == ',') {
			tmp[i] = '\0';
			(*nr_allowed_ips)++;
		}
	}

	(*nr_allowed_ips)++;
	arr = calloc(*nr_allowed_ips, sizeof(char *));
	if (!arr) {
		free(tmp);
		return -ENOMEM;
	}

	for (i = 0; i < *nr_allowed_ips; i++) {
		arr[i] = strdup(tmp);
		if (!arr[i]) {
			while (i--)
				free(arr[i]);
			free(arr);
			free(tmp);
			return -ENOMEM;
		}

		tmp += strlen(tmp) + 1;
	}

	*allowed_ips = arr;
	free(tmp);

	return 0;
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
		printf("Error: failed to load interface %s\n", arg.ifname);
		return ret;
	}

	ret = wgm_parse_allowed_ips(arg.allowed_ips, &peer.allowed_ips, &peer.nr_allowed_ips);
	if (ret < 0) {
		printf("Error: failed to parse allowed IPs\n");
		return ret;
	}

	strncpyl(peer.public_key, arg.public_key, sizeof(peer.public_key));
	strncpyl(peer.bind_ip, arg.bind_ip, sizeof(peer.bind_ip));

	ret = wgm_peer_append(&iface, &peer);
	if (ret < 0) {
		printf("Error: failed to append peer\n");
		return ret;
	}

	wgm_iface_dump(&iface);
	ret = wgm_iface_save(ctx, &iface);
	wgm_iface_free(&iface);
	return ret;
}

int wgm_peer_update(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}

json_object *wgm_peer_array_to_json(const struct wgm_peer *peers, uint16_t nr_peers)
{
	json_object *jarr;
	size_t i;

	jarr = json_object_new_array();
	if (!jarr)
		return NULL;

	for (i = 0; i < nr_peers; i++) {
		json_object *jpeer, *tmp;

		jpeer = json_object_new_object();
		if (!jpeer) {
			json_object_put(jarr);
			return NULL;
		}

		json_object_object_add(jpeer, "public_key", json_object_new_string(peers[i].public_key));
		json_object_object_add(jpeer, "bind_ip", json_object_new_string(peers[i].bind_ip));
		tmp = json_object_new_from_str_array(peers[i].allowed_ips, peers[i].nr_allowed_ips);
		json_object_object_add(jpeer, "allowed_ips", tmp);

		json_object_array_add(jarr, jpeer);
	}

	return jarr;
}

void wgm_peer_free(struct wgm_peer *peer)
{
	free_str_array(peer->allowed_ips, peer->nr_allowed_ips);
	memset(peer, 0, sizeof(*peer));
}

int wgm_peer_from_json(struct wgm_peer *peer, json_object *jobj)
{
	json_object *jtmp;
	const char *str;
	int ret;

	ret = json_object_object_get_ex(jobj, "public_key", &jtmp);
	if (!ret) {
		printf("Error: missing 'public_key' field\n");
		return -EINVAL;
	}

	str = json_object_get_string(jtmp);
	if (!str || strlen(str) >= sizeof(peer->public_key)) {
		printf("Error: invalid 'public_key' field\n");
		return -EINVAL;
	}

	strncpyl(peer->public_key, str, sizeof(peer->public_key));

	ret = json_object_object_get_ex(jobj, "bind_ip", &jtmp);
	if (!ret) {
		printf("Error: missing 'bind_ip' field\n");
		return -EINVAL;
	}

	str = json_object_get_string(jtmp);

	if (inet_pton(AF_INET, str, peer->bind_ip) <= 0) {
		if (inet_pton(AF_INET6, str, peer->bind_ip) <= 0) {
			printf("Error: invalid IP address: %s\n", str);
			return -EINVAL;
		}
	}

	ret = load_str_array_from_json(&peer->allowed_ips, &peer->nr_allowed_ips, jobj);
	if (ret) {
		printf("Error: failed to load 'allowed_ips' field\n");
		return ret;
	}

	return 0;
}

int wgm_peer_array_from_json(struct wgm_peer **peers, uint16_t *nr_peers, json_object *jarr)
{
	size_t i;
	int ret;

	*nr_peers = 0;
	*peers = NULL;

	if (!json_object_is_type(jarr, json_type_array)) {
		printf("Error: invalid 'peers' field\n");
		return -EINVAL;
	}

	*nr_peers = json_object_array_length(jarr);
	if (!*nr_peers)
		return 0;

	*peers = calloc(*nr_peers, sizeof(struct wgm_peer));
	if (!*peers)
		return -ENOMEM;

	for (i = 0; i < *nr_peers; i++) {
		ret = wgm_peer_from_json(&(*peers)[i], json_object_array_get_idx(jarr, i));
		if (ret) {
			free_str_array((*peers)[i].allowed_ips, (*peers)[i].nr_allowed_ips);
			free(*peers);
			*peers = NULL;
			*nr_peers = 0;
			return ret;
		}
	}

	return 0;
}
#endif

int wgm_peer_add(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}

void wgm_peer_dump(const struct wgm_peer *peer)
{
	printf("Peer:\n");
	printf("  Public key: %s\n", peer->public_key);
	printf("  Bind IP: %s\n", peer->bind_ip);
	printf("  Allowed IPs:\n");
	wgm_str_array_dump(&peer->allowed_ips);
}
