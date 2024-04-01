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
	WGM_IFACE_ARG_HAS_IFNAME	= (1ull << 0ull),
	WGM_IFACE_ARG_HAS_LISTEN_PORT	= (1ull << 1ull),
	WGM_IFACE_ARG_HAS_PRIVATE_KEY	= (1ull << 2ull),
};

static const char wgm_iface_opt_short[] = "i:l:k:hf";

static const struct option wgm_iface_opt_long[] = {
	{"ifname",		required_argument,	NULL,	'i'},
	{"listen-port",		required_argument,	NULL,	'l'},
	{"private-key",		required_argument,	NULL,	'k'},
	{"help",		no_argument,		NULL,	'h'},
	{"force",		no_argument,		NULL,	'f'},
	{NULL,			0,			NULL,	0},
};

static void wgm_iface_add_help(const char *app)
{
	printf("Usage: %s create [options]\n", app);
	printf("Options:\n");
	printf("  -i, --ifname=IFNAME\t\tInterface name\n");
	printf("  -l, --listen-port=PORT\tListen port\n");
	printf("  -k, --private-key=KEY\tPrivate key\n");
	printf("  -h, --help\t\t\tDisplay this help\n");
	printf("  -f, --force\t\t\tForce update if error or interface exists\n");
}

static int wgm_parse_port(const char *port, uint16_t *value)
{
	char *endptr;
	long val;

	val = strtol(port, &endptr, 10);
	if (*endptr != '\0' || val < 0 || val > UINT16_MAX) {
		printf("Error: invalid port number: %s\n", port);
		return -1;
	}

	*value = (uint16_t)val;
	return 0;
}

static int wgm_create_getopt(int argc, char *argv[], struct wgm_iface_arg *arg)
{
	uint64_t flags = 0;
	int c;

	arg->force = false;

	while (1) {
		c = getopt_long(argc, argv, wgm_iface_opt_short, wgm_iface_opt_long, NULL);
		if (c < 0)
			break;

		switch (c) {
		case 'i':
			if (wgm_parse_ifname(optarg, arg->ifname) < 0)
				return -EINVAL;
			flags |= WGM_IFACE_ARG_HAS_IFNAME;
			break;
		case 'l':
			if (wgm_parse_port(optarg, &arg->listen_port) < 0)
				return -EINVAL;
			flags |= WGM_IFACE_ARG_HAS_LISTEN_PORT;
			break;
		case 'k':
			if (wgm_parse_key(optarg, arg->private_key, sizeof(arg->private_key)))
				return -EINVAL;
			flags |= WGM_IFACE_ARG_HAS_PRIVATE_KEY;
			break;
		case 'f':
			arg->force = true;
			break;
		case 'h':
			wgm_iface_add_help(argv[0]);
			return -1;
		case '?':
			return -EINVAL;
		}
	}

	if (!(flags & WGM_IFACE_ARG_HAS_IFNAME)) {
		printf("Error: missing interface name\n");
		return -1;
	}

	if (!(flags & WGM_IFACE_ARG_HAS_LISTEN_PORT)) {
		printf("Error: missing listen port\n");
		return -1;
	}

	if (!(flags & WGM_IFACE_ARG_HAS_PRIVATE_KEY)) {
		printf("Error: missing private key\n");
		return -1;
	}

	return 0;
}

static char *wgm_iface_to_json_str(const struct wgm_iface *iface)
{
	json_object *jobj, *tmp;
	char *jstr;
	int ret;

	jobj = json_object_new_object();
	if (!jobj)
		return NULL;

	tmp = json_object_new_string(iface->ifname);
	json_object_object_add(jobj, "ifname", tmp);

	tmp = json_object_new_int(iface->listen_port);
	json_object_object_add(jobj, "listen_port", tmp);

	tmp = json_object_new_string(iface->private_key);
	json_object_object_add(jobj, "private_key", tmp);

	ret = wgm_str_array_to_json(&iface->addresses, &tmp);
	if (!ret)
		json_object_object_add(jobj, "addresses", tmp);
	else
		wgm_log_err("Error: wgm_iface_to_json_str: failed to convert addresses to JSON\n");

	ret = wgm_str_array_to_json(&iface->allowed_ips, &tmp);
	if (!ret)
		json_object_object_add(jobj, "allowed_ips", tmp);
	else
		wgm_log_err("Error: wgm_iface_to_json_str: failed to convert allowed IPs to JSON\n");

	jstr = strdup(json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY));
	if (!jstr)
		wgm_log_err("Error: wgm_iface_to_json_str: failed to allocate memory\n");

	json_object_put(jobj);
	return jstr;
}

static int wgm_iface_load_from_json_str(struct wgm_iface *iface, const char *jstr)
{
	json_object *jobj, *tmp;
	const char *tstr;
	int port;
	int ret;

	jobj = json_tokener_parse(jstr);
	if (!jobj) {
		wgm_log_err("Error: wgm_iface_load_from_json_str: failed to parse JSON object\n");
		return -EINVAL;
	}


	tmp = json_object_object_get(jobj, "ifname");
	if (!tmp || !json_object_is_type(tmp, json_type_string)) {
		wgm_log_err("Error: wgm_iface_load_from_json_str: missing or invalid 'ifname' field\n");
		ret = -EINVAL;
		goto out;
	}
	tstr = json_object_get_string(tmp);
	if (!tstr || strlen(tstr) >= sizeof(iface->ifname)) {
		wgm_log_err("Error: wgm_iface_load_from_json_str: invalid 'ifname' field\n");
		ret = -EINVAL;
		goto out;
	}
	strncpyl(iface->ifname, tstr, sizeof(iface->ifname));


	tmp = json_object_object_get(jobj, "listen_port");
	if (!tmp || !json_object_is_type(tmp, json_type_int)) {
		wgm_log_err("Error: wgm_iface_load_from_json_str: missing or invalid 'listen_port' field\n");
		ret = -EINVAL;
		goto out;
	}

	port = json_object_get_int(tmp);
	if (port < 0 || port > 65535) {
		wgm_log_err("Error: wgm_iface_load_from_json_str: invalid 'listen_port' field\n");
		ret = -EINVAL;
		goto out;
	}
	iface->listen_port = (uint16_t)port;


	tmp = json_object_object_get(jobj, "private_key");
	if (!tmp || !json_object_is_type(tmp, json_type_string)) {
		wgm_log_err("Error: wgm_iface_load_from_json_str: missing or invalid 'private_key' field\n");
		ret = -EINVAL;
		goto out;
	}

	tstr = json_object_get_string(tmp);
	if (!tstr || strlen(tstr) >= sizeof(iface->private_key)) {
		wgm_log_err("Error: wgm_iface_load_from_json_str: invalid 'private_key' field\n");
		ret = -EINVAL;
		goto out;
	}
	strncpyl(iface->private_key, tstr, sizeof(iface->private_key));

	ret = 0;
out:
	json_object_put(jobj);
	return ret;
}

int wgm_iface_load(struct wgm_ctx *ctx, struct wgm_iface *iface, const char *ifname)
{
	size_t len, read_ret;
	char path[8192];
	char *jstr;
	FILE *fp;
	int ret;

	snprintf(path, sizeof(path), "%s/%s.json", ctx->data_dir, ifname);
	fp = fopen(path, "rb");
	if (!fp)
		return -errno;

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	jstr = malloc(len + 1);
	if (!jstr) {
		fclose(fp);
		return -ENOMEM;
	}

	read_ret = fread(jstr, 1, len, fp);
	fclose(fp);
	if (read_ret != len) {
		free(jstr);
		return -EIO;
	}

	jstr[len] = '\0';
	ret = wgm_iface_load_from_json_str(iface, jstr);
	free(jstr);
	return ret;
}

int wgm_iface_save(struct wgm_ctx *ctx, const struct wgm_iface *iface)
{
	char path[8192];
	char *jstr;
	FILE *fp;
	int ret;

	ret = snprintf(path, sizeof(path), "%s/%s.json", ctx->data_dir, iface->ifname);
	if (ret > (int)sizeof(path)) {
		fprintf(stderr, "Error: path is too long: %s\n", path);
		return -ENAMETOOLONG;
	}

	fp = fopen(path, "wb");
	if (!fp) {
		ret = -errno;
		fprintf(stderr, "Error: failed to open file: %s\n", path);
		return ret;
	}

	jstr = wgm_iface_to_json_str(iface);
	if (!jstr) {
		fclose(fp);
		return -ENOMEM;
	}

	fputs(jstr, fp);
	fputc('\n', fp);
	fclose(fp);
	return 0;
}

int wgm_iface_update(int argc, char *argv[], struct wgm_ctx *ctx)
{
	(void)argc;
	(void)argv;
	(void)ctx;
	return 0;
}

/*
 * "iface add" rules:
 *
 *   1) The interface name is the primary key (must be unique).
 *
 *   2) The listen port must be unique (not used by other interfaces).
 * 
 *   3) If the interface name already exists, it will try to update the
 *      listen port and private key.
 */
int wgm_iface_add(int argc, char *argv[], struct wgm_ctx *ctx)
{
	struct wgm_iface_arg arg;
	struct wgm_iface iface;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));
	ret = wgm_create_getopt(argc, argv, &arg);
	if (ret < 0)
		return -1;

	ret = wgm_iface_load(ctx, &iface, arg.ifname);
	if (!ret) {
		if (!arg.force) {
			printf("Error: interface already exists, use --force to force update\n");
			return -EEXIST;
		}

		wgm_iface_dump(&iface);
		wgm_iface_save(ctx, &iface);
		return wgm_iface_update(argc, argv, ctx);
	}

	if (ret != -ENOENT) {
		if (!arg.force)
			return ret;

		printf("Force flag is set, ignoring error...\n");
	}

	strncpyl(iface.ifname, arg.ifname, IFNAMSIZ);
	iface.listen_port = arg.listen_port;
	strncpyl(iface.private_key, arg.private_key, sizeof(iface.private_key));
	wgm_iface_dump(&iface);
	return wgm_iface_save(ctx, &iface);
}

void wgm_iface_free(struct wgm_iface *iface)
{
	wgm_str_array_free(&iface->addresses);
	wgm_str_array_free(&iface->allowed_ips);
	wgm_peer_array_free(&iface->peers);
}

void wgm_iface_dump(const struct wgm_iface *iface)
{
	printf("Interface: %s\n", iface->ifname);
	printf("  Listen port: %u\n", iface->listen_port);
	printf("  Private key: %s\n", iface->private_key);
	printf("  Addresses:\n");
	wgm_str_array_dump(&iface->addresses);
	printf("  Allowed IPs:\n");
	wgm_str_array_dump(&iface->allowed_ips);
	printf("  Peers:\n");
	wgm_peer_array_dump(&iface->peers);
}

void wgm_peer_array_dump(const struct wgm_peer_array *peers)
{
	size_t i;

	for (i = 0; i < peers->nr; i++) {
		printf("  Peer %zu:\n", i);
		wgm_peer_dump(&peers->peers[i]);
	}
}

void wgm_peer_array_free(struct wgm_peer_array *peers)
{
	size_t i;

	if (!peers)
		return;

	for (i = 0; i < peers->nr; i++)
		wgm_peer_free(&peers->peers[i]);

	free(peers->peers);
	memset(peers, 0, sizeof(*peers));
}

int wgm_iface_add_peer(struct wgm_iface *iface, const struct wgm_peer *peer)
{
	struct wgm_peer *tmp;
	size_t i;

	tmp = realloc(iface->peers.peers, (iface->peers.nr + 1) * sizeof(*tmp));
	if (!tmp)
		return -ENOMEM;

	i = iface->peers.nr;
	iface->peers.peers = tmp;
	iface->peers.nr++;
	iface->peers.peers[i] = *peer;
	return 0;
}

int wgm_iface_del_peer(struct wgm_iface *iface, const char *public_key)
{
	struct wgm_peer *tmp;
	size_t i, j;

	for (i = 0; i < iface->peers.nr; i++) {
		if (strcmp(iface->peers.peers[i].public_key, public_key) == 0)
			break;
	}

	if (i == iface->peers.nr)
		return -ENOENT;

	tmp = malloc((iface->peers.nr - 1) * sizeof(*tmp));
	if (!tmp)
		return -ENOMEM;

	for (j = 0; j < iface->peers.nr; j++) {
		if (j < i)
			tmp[j] = iface->peers.peers[j];
		else if (j > i)
			tmp[j - 1] = iface->peers.peers[j];
	}

	free(iface->peers.peers);
	iface->peers.peers = tmp;
	iface->peers.nr--;
	return 0;

}

int wgm_iface_update_peer(struct wgm_iface *iface, const struct wgm_peer *peer)
{
	size_t i;

	for (i = 0; i < iface->peers.nr; i++) {
		if (strcmp(iface->peers.peers[i].public_key, peer->public_key) == 0) {
			iface->peers.peers[i] = *peer;
			return 0;
		}
	}

	return -ENOENT;
}
