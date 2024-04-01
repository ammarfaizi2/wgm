// SPDX-License-Identifier: GPL-2.0-only

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <linux/if.h>
#include <json-c/json.h>
#include <arpa/inet.h>

struct wgm_create_arg {
	char		ifname[IFNAMSIZ];
	uint16_t	listen_port;
	char		private_key[128];
};

struct wgm_peer {
	char		public_key[128];
	char		**allowed_ips;
	uint16_t	nr_allowed_ips;
};

struct wgm_iface {
	char		ifname[IFNAMSIZ];
	uint16_t	listen_port;
	char		private_key[128];

	char		**addresses;
	uint16_t	nr_addresses;

	char		**allowed_ips;
	uint16_t	nr_allowed_ips;

	struct wgm_peer	*peers;
	uint16_t	nr_peers;
};

enum {
	WGM_CREATE_HAS_IFNAME		= (1ull << 0ull),
	WGM_CREATE_HAS_LISTEN_PORT	= (1ull << 1ull),
	WGM_CREATE_HAS_PRIVATE_KEY	= (1ull << 2ull),
};

static char data_dir[4096] = "./wgm_data";

static const struct option wgm_create_options[] = {
	{"ifname",		required_argument,	NULL,	'i'},
	{"listen-port",		required_argument,	NULL,	'l'},
	{"private-key",		required_argument,	NULL,	'k'},
	{"help",		no_argument,		NULL,	'h'},
	{NULL,			0,			NULL,	0},
};

static const struct option wgm_add_peer_options[] = {
	{"ifname",		required_argument,	NULL,	'i'},
	{"public-key",		required_argument,	NULL,	'p'},
	{"allowed-ips",		required_argument,	NULL,	'a'},
	{"help",		no_argument,		NULL,	'h'},
	{NULL,			0,			NULL,	0},
};

static char *strncpy2(char *dest, const char *src, size_t n)
{
	strncpy(dest, src, n - 1);
	dest[n - 1] = '\0';
	return dest;
}

static json_object *json_object_new_from_str_array(char **array, uint16_t nr)
{
	json_object *jobj;
	size_t i;

	jobj = json_object_new_array();
	if (!jobj)
		return NULL;

	for (i = 0; i < nr; i++)
		json_object_array_add(jobj, json_object_new_string(array[i]));

	return jobj;
}

static int load_str_array_from_json(char ***array, uint16_t *nr, const json_object *jobj)
{
	json_object *tmp;
	const char *tstr;
	size_t i;

	*nr = json_object_array_length(jobj);
	if (!*nr)
		return 0;

	*array = malloc(*nr * sizeof(char *));
	if (!*array)
		return -ENOMEM;

	for (i = 0; i < *nr; i++) {
		tmp = json_object_array_get_idx(jobj, i);
		if (!tmp || !json_object_is_type(tmp, json_type_string)) {
			fprintf(stderr, "Error: invalid array element\n");
			goto err;
		}

		tstr = json_object_get_string(tmp);
		if (!tstr) {
			fprintf(stderr, "Error: invalid array element\n");
			goto err;
		}

		(*array)[i] = strdup(tstr);
		if (!(*array)[i]) {
			fprintf(stderr, "Error: failed to allocate memory\n");
			goto err;
		}
	}

	return 0;

err:
	while (i--)
		free((*array)[i]);

	free(*array);
	return -ENOMEM;
}

static void free_str_array(char **array, uint16_t nr)
{
	size_t i;

	for (i = 0; i < nr; i++)
		free(array[i]);

	free(array);
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

	strncpy2(iface->ifname, tstr, IFNAMSIZ);

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

	strncpy2(iface->private_key, tstr, sizeof(iface->private_key));

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

static int wgm_load_iface(struct wgm_iface *iface, const char *ifname)
{
	size_t len, read_ret;
	char path[8192];
	char *jstr;
	FILE *fp;
	int ret;

	snprintf(path, sizeof(path), "%s/%s.json", data_dir, ifname);
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

static int wgm_save_iface(const struct wgm_iface *iface)
{
	json_object *jobj, *tmp;
	char path[8192];
	FILE *fp;
	int ret;

	jobj = json_object_new_object();
	if (!jobj) {
		fprintf(stderr, "Error: failed to create JSON object\n");
		return -ENOMEM;
	}

	tmp = json_object_new_string(iface->ifname);
	json_object_object_add(jobj, "ifname", tmp);

	tmp = json_object_new_int(iface->listen_port);
	json_object_object_add(jobj, "listen_port", tmp);

	tmp = json_object_new_string(iface->private_key);
	json_object_object_add(jobj, "private_key", tmp);

	tmp = json_object_new_from_str_array(iface->addresses, iface->nr_addresses);
	json_object_object_add(jobj, "addresses", tmp);

	tmp = json_object_new_from_str_array(iface->allowed_ips, iface->nr_allowed_ips);
	json_object_object_add(jobj, "allowed_ips", tmp);

	tmp = json_object_new_array();
	json_object_object_add(jobj, "peers", tmp);

	snprintf(path, sizeof(path), "%s/%s.json", data_dir, iface->ifname);
	fp = fopen(path, "wb");
	if (!fp) {
		ret = -errno;
		fprintf(stderr, "Error: failed to open file: %s\n", path);
		json_object_put(jobj);
		return ret;
	}

	fputs(json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY), fp);
	fclose(fp);
	json_object_put(jobj);
	return 0;
}

static void wgm_create_help(const char *app)
{
	printf("Usage: %s create [options]\n", app);
	printf("Options:\n");
	printf("  -i, --ifname=IFNAME\t\tInterface name\n");
	printf("  -l, --listen-port=PORT\tListen port\n");
	printf("  -k, --private-key=KEY\tPrivate key\n");
	printf("  -h, --help\t\t\tDisplay this help\n");
}

static int mkdir_recursive(const char *path, mode_t mode)
{
	char *p, *tmp;
	int ret;

	tmp = strdup(path);
	if (!tmp)
		return -ENOMEM;

	p = tmp;
	while (*p) {
		if (*p != '/') {
			p++;
			continue;
		}

		*p = '\0';
		ret = mkdir(tmp, mode);
		if (ret < 0) {
			ret = -errno;
			if (ret != -EEXIST) {
				free(tmp);
				return ret;
			}
		}
		*p = '/';
		p++;
	}

	ret = mkdir(tmp, mode);
	free(tmp);
	if (ret < 0) {
		ret = -errno;
		if (ret != -EEXIST)
			return ret;
	}

	return 0;
}

static int wgm_init(void)
{
	const char *data_dir_env;
	int ret;

	data_dir_env = getenv("WGM_DATA_DIR");
	if (data_dir_env) {
		strncpy(data_dir, data_dir_env, sizeof(data_dir) - 1);
		data_dir[sizeof(data_dir) - 1] = '\0';
	}

	ret = mkdir_recursive(data_dir, 0755);
	if (ret < 0 && ret != -EEXIST) {
		fprintf(stderr, "Error: failed to create data directory: %s: %s\n", data_dir, strerror(-ret));
		return -1;
	}

	return 0;
}

static int wgm_create_getopt(int argc, char *argv[], struct wgm_create_arg *arg)
{
	uint64_t flags = 0;
	int c;

	while (1) {
		c = getopt_long(argc, argv, "i:l:k:h", wgm_create_options, NULL);
		if (c < 0)
			break;

		switch (c) {
		case 'i':
			if (wgm_parse_ifname(optarg, arg->ifname) < 0)
				return -1;
			flags |= WGM_CREATE_HAS_IFNAME;
			break;
		case 'l':
			if (wgm_parse_port(optarg, &arg->listen_port) < 0)
				return -1;
			flags |= WGM_CREATE_HAS_LISTEN_PORT;
			break;
		case 'k':
			if (wgm_parse_private_key(optarg, arg->private_key, sizeof(arg->private_key)) < 0)
				return -1;
			flags |= WGM_CREATE_HAS_PRIVATE_KEY;
			break;
		case 'h':
			wgm_create_help(argv[0]);
			return -1;
		case '?':
			return -1;
		}
	}

	if (!(flags & WGM_CREATE_HAS_IFNAME)) {
		printf("Error: missing interface name\n");
		return -1;
	}

	if (!(flags & WGM_CREATE_HAS_LISTEN_PORT)) {
		printf("Error: missing listen port\n");
		return -1;
	}

	if (!(flags & WGM_CREATE_HAS_PRIVATE_KEY)) {
		printf("Error: missing private key\n");
		return -1;
	}

	return 0;
}

static int wgm_add_peer_getopt(int argc, char *argv[], struct wgm_peer *peer)
{
	int c;

	while (1) {
		c = getopt_long(argc, argv, "i:p:a:h", wgm_add_peer_options, NULL);
		if (c < 0)
			break;

		switch (c) {
		case 'i':
			break;
		case 'p':
			break;
		case 'a':
			break;
		case 'h':
			break;
		case '?':
			break;
		}
	}

	return 0;
}

static int wgm_update(const struct wgm_create_arg *arg, struct wgm_iface *iface)
{
	return 0;
}

static int wgm_create(int argc, char *argv[])
{
	struct wgm_create_arg arg;
	struct wgm_iface iface;
	int ret;

	/*
	 * Create action rules:
	 *
	 *   1) The interface name is the primary key (must be unique).
	 *
	 *   2) The listen port must be unique (not used by other interfaces).
	 * 
	 *   3) If the interface name already exists, it will try to update the
	 *      listen port and private key.
	 */
	memset(&arg, 0, sizeof(arg));
	ret = wgm_create_getopt(argc, argv, &arg);
	if (ret < 0)
		return 1;

	ret = wgm_init();
	if (ret < 0)
		return 1;

	ret = wgm_load_iface(&iface, arg.ifname);
	if (!ret)
		return wgm_update(&arg, &iface);

	if (ret != -ENOENT) {
		fprintf(stderr, "Error: failed to load interface: %s: %s\n", arg.ifname, strerror(-ret));
		return 1;
	}

	iface.listen_port = arg.listen_port;
	strncpy2(iface.ifname, arg.ifname, IFNAMSIZ);
	strncpy2(iface.private_key, arg.private_key, sizeof(iface.private_key));
	return wgm_save_iface(&iface);
}

static int wgm_add_peer(int argc, char *argv[])
{
	struct wgm_iface iface;
	struct wgm_peer peer;
	int ret;

	ret = wgm_init();
	if (ret < 0)
		return 1;

	ret = wgm_load_iface(&iface, argv[2]);
	if (ret) {
		fprintf(stderr, "Error: failed to load interface: %s: %s\n", argv[2], strerror(-ret));
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		fprintf(stderr, "Usage: %s <command> [options]\n", argv[0]);
		return 1;
	}

	if (!strcmp(argv[1], "create"))
		return wgm_create(argc - 1, argv + 1);

	if (!strcmp(argv[1], "add-peer"))
		return wgm_add_peer(argc, argv);

	// if (!strcmp(argv[1], "del-peer"))
	// 	return wgm_del_peer(argc, argv);

	printf("Error: unknown command: %s\n", argv[1]);
	return 1;
}
