// SPDX-License-Identifier: GPL-2.0-only
#include "wg_iface.h"
#include "wg_peer.h"

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

static const char wgm_iface_opt_short[] = "i:l:k:h";

static const struct option wgm_iface_opt_long[] = {
	{"ifname",		required_argument,	NULL,	'i'},
	{"listen-port",		required_argument,	NULL,	'l'},
	{"private-key",		required_argument,	NULL,	'k'},
	{"help",		no_argument,		NULL,	'h'},
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
}

static int wgm_parse_ifname(const char *ifname, char *buf)
{
	size_t i, len;

	if (strlen(ifname) >= IFNAMSIZ) {
		printf("Error: interface name is too long, max length is %d\n", IFNAMSIZ - 1);
		return -1;
	}

	strncpy(buf, ifname, IFNAMSIZ - 1);
	buf[IFNAMSIZ - 1] = '\0';

	/*
	 * Validate the interface name:
	 *   - Must be a valid interface name.
	 *   - Valid characters: [a-zA-Z0-9\-]
	 */
	len = strlen(buf);
	for (i = 0; i < len; i++) {
		if ((buf[i] >= 'a' && buf[i] <= 'z') ||
		    (buf[i] >= 'A' && buf[i] <= 'Z') ||
		    (buf[i] >= '0' && buf[i] <= '9') ||
		    buf[i] == '-') {
			continue;
		}

		printf("Error: invalid interface name: %s\n", buf);
		return -1;
	}

	return 0;
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

static int wgm_parse_private_key(const char *key, char *buf, size_t size)
{
	if (strlen(key) >= size) {
		printf("Error: private key is too long, max length is %zu\n", size - 1);
		return -1;
	}

	strncpy(buf, key, size - 1);
	buf[size - 1] = '\0';
	return 0;
}


static int wgm_create_getopt(int argc, char *argv[], struct wgm_iface_arg *arg)
{
	uint64_t flags = 0;
	int c;

	while (1) {
		c = getopt_long(argc, argv, wgm_iface_opt_short, wgm_iface_opt_long, NULL);
		if (c < 0)
			break;

		switch (c) {
		case 'i':
			if (wgm_parse_ifname(optarg, arg->ifname) < 0)
				return -1;
			flags |= WGM_IFACE_ARG_HAS_IFNAME;
			break;
		case 'l':
			if (wgm_parse_port(optarg, &arg->listen_port) < 0)
				return -1;
			flags |= WGM_IFACE_ARG_HAS_LISTEN_PORT;
			break;
		case 'k':
			if (wgm_parse_private_key(optarg, arg->private_key, sizeof(arg->private_key)) < 0)
				return -1;
			flags |= WGM_IFACE_ARG_HAS_PRIVATE_KEY;
			break;
		case 'h':
			wgm_iface_add_help(argv[0]);
			return -1;
		case '?':
			return -1;
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

static int wgm_load_iface_json_str(struct wgm_iface *iface, const char *jstr)
{
	json_object *jobj, *tmp;
	const char *tstr;
	int port;
	int ret;

	jobj = json_tokener_parse(jstr);
	if (!jobj) {
		fprintf(stderr, "Error: failed to parse JSON\n");
		return -EINVAL;
	}

	tmp = json_object_object_get(jobj, "ifname");
	if (!tmp || !json_object_is_type(tmp, json_type_string)) {
		fprintf(stderr, "Error: missing or invalid 'ifname' field\n");
		ret = -EINVAL;
		goto out;
	}

	tstr = json_object_get_string(tmp);
	if (!tstr || strlen(tstr) >= IFNAMSIZ) {
		fprintf(stderr, "Error: invalid 'ifname' field\n");
		ret = -EINVAL;
		goto out;
	}

	strncpyl(iface->ifname, tstr, IFNAMSIZ);

	tmp = json_object_object_get(jobj, "listen_port");
	if (!tmp || !json_object_is_type(tmp, json_type_int)) {
		fprintf(stderr, "Error: missing or invalid 'listen_port' field\n");
		ret = -EINVAL;
		goto out;
	}

	port = json_object_get_int(tmp);
	if (port < 0 || port > 65535) {
		fprintf(stderr, "Error: invalid 'listen_port' field\n");
		ret = -EINVAL;
		goto out;
	}

	iface->listen_port = (uint16_t)port;

	tmp = json_object_object_get(jobj, "private_key");
	if (!tmp || !json_object_is_type(tmp, json_type_string)) {
		fprintf(stderr, "Error: missing or invalid 'private_key' field\n");
		ret = -EINVAL;
		goto out;
	}

	tstr = json_object_get_string(tmp);
	if (!tstr || strlen(tstr) >= sizeof(iface->private_key)) {
		fprintf(stderr, "Error: invalid 'private_key' field\n");
		ret = -EINVAL;
		goto out;
	}

	strncpyl(iface->private_key, tstr, sizeof(iface->private_key));

	tmp = json_object_object_get(jobj, "addresses");
	if (!tmp || !json_object_is_type(tmp, json_type_array)) {
		fprintf(stderr, "Error: missing or invalid 'addresses' field\n");
		ret = -EINVAL;
		goto out;
	}

	ret = load_str_array_from_json(&iface->addresses, &iface->nr_addresses, tmp);
	if (ret) {
		fprintf(stderr, "Error: failed to load 'addresses' field\n");
		goto out;
	}

	tmp = json_object_object_get(jobj, "allowed_ips");
	if (!tmp || !json_object_is_type(tmp, json_type_array)) {
		fprintf(stderr, "Error: missing or invalid 'allowed_ips' field\n");
		free_str_array(iface->addresses, iface->nr_addresses);
		ret = -EINVAL;
		goto out;
	}

	ret = load_str_array_from_json(&iface->allowed_ips, &iface->nr_allowed_ips, tmp);
	if (ret) {
		fprintf(stderr, "Error: failed to load 'allowed_ips' field\n");
		free_str_array(iface->addresses, iface->nr_addresses);
		goto out;
	}

	ret = 0;
out:
	json_object_put(jobj);
	return ret;
}

static int wgm_load_iface(struct wgm_ctx *ctx, struct wgm_iface *iface,
			  const char *ifname)
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
	ret = wgm_load_iface_json_str(iface, jstr);
	free(jstr);
	return ret;
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

	ret = wgm_create_getopt(argc, argv, &arg);
	if (ret < 0)
		return -1;

	ret = wgm_load_iface(ctx, &iface, arg.ifname);
	return ret;
}
