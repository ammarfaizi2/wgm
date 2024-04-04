// SPDX-License-Identifier: GPL-2.0-only
#include "wgm_iface.h"
#include "wgm_peer.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json.h>

struct priv_arg {
	uint64_t flags;
	const char *name;
};

static const struct priv_arg aarg[] = {
#define ARG_IFACE	(1ull << 0ull)
	{ARG_IFACE, "--ifname"},

#define ARG_LISTEN_PORT	(1ull << 1ull)
	{ARG_LISTEN_PORT, "--listen-port"},

#define ARG_PRIVATE_KEY	(1ull << 2ull)
	{ARG_PRIVATE_KEY, "--private-key"},

#define ARG_ALLOWED_IP	(1ull << 3ull)
	{ARG_ALLOWED_IP, "--allowed-ip"},

#define ARG_MTU		(1ull << 4ull)
	{ARG_MTU, "--mtu"},

#define ARG_ADDRESS	(1ull << 5ull)
	{ARG_ADDRESS, "--address"},

#define ARG_FORCE	(1ull << 63ull)
	{ARG_FORCE, "--force"},
};

static const struct option wgm_iface_opt_long[] = {
	{"ifname",		required_argument,	NULL,	'i'},
	{"listen-port",		required_argument,	NULL,	'l'},
	{"private-key",		required_argument,	NULL,	'k'},
	{"mtu",			required_argument,	NULL,	'm'},
	{"address",		required_argument,	NULL,	'a'},
	{"allowed-ip",		required_argument,	NULL,	'p'},
	{"help",		no_argument,		NULL,	'h'},
	{"force",		no_argument,		NULL,	'f'},
	{NULL,			0,			NULL,	0},
};
static const char wgm_iface_opt_short[] = "i:l:k:m:a:p:hf";

static void wgm_iface_help(const char *app)
{
	printf("Usage: %s [options]\n\n", app);
	printf("Options:\n");
	printf("  -i, --ifname <name>       Interface name\n");
	printf("  -l, --listen-port <port>  Listen port\n");
	printf("  -k, --private-key <key>   Private key\n");
	printf("  -m, --mtu <size>          MTU\n");
	printf("  -a, --address <addr>      Address (comma separated for multiple values)\n");
	printf("  -p, --allowed-ip <addr>   Allowed IP (comma separated for multiple values)\n");
	printf("  -f, --force               Force operation\n");
	printf("  -h, --help                Show this help message\n");
}

static int wgm_parse_port(const char *port, uint16_t *value)
{
	char *endptr;
	long val;

	val = strtol(port, &endptr, 10);
	if (*endptr != '\0' || val < 0 || val > UINT16_MAX) {
		wgm_log_err("Error: invalid port number: %s\n", port);
		return -1;
	}

	*value = (uint16_t)val;
	return 0;
}

static int wgm_iface_getopt(int argc, char *argv[], struct wgm_iface_arg *arg,
			     uint64_t allowed_args, uint64_t required_args,
			     uint64_t *out_flags)
{
	uint64_t flags = 0;
	size_t i;
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
			flags |= ARG_IFACE;
			break;
		case 'l':
			if (wgm_parse_port(optarg, &arg->listen_port) < 0)
				return -EINVAL;
			flags |= ARG_LISTEN_PORT;
			break;
		case 'k':
			if (wgm_parse_key(optarg, arg->private_key, sizeof(arg->private_key)))
				return -EINVAL;
			flags |= ARG_PRIVATE_KEY;
			break;
		case 'm':
			if (wgm_parse_mtu(optarg, &arg->mtu) < 0)
				return -EINVAL;
			flags |= ARG_MTU;
			break;
		case 'a':
			if (wgm_parse_csv(&arg->addresses, optarg) < 0)
				return -EINVAL;
			flags |= ARG_ADDRESS;
			break;
		case 'p':
			if (wgm_parse_csv(&arg->allowed_ips, optarg) < 0)
				return -EINVAL;
			flags |= ARG_ALLOWED_IP;
			break;
		case 'f':
			arg->force = true;
			flags |= ARG_FORCE;
			break;
		case 'h':
			wgm_iface_help(argv[0]);
			return -1;
		case '?':
			return -EINVAL;
		}
	}

	for (i = 0; i < sizeof(aarg) / sizeof(aarg[0]); i++) {

		if (!(aarg[i].flags & allowed_args)) {
			if (flags & aarg[i].flags) {
				wgm_log_err("\nError: wgm_iface_getopt: argument %s is not allowed for this command\n", aarg[i].name);
				return -EINVAL;
			}

			continue;
		}

		if (!(aarg[i].flags & required_args))
			continue;

		if (!(flags & aarg[i].flags)) {
			wgm_log_err("\nError: wgm_iface_getopt: missing required argument: %s\n\n", aarg[i].name);
			wgm_iface_help(argv[0]);
			return -EINVAL;
		}
	}

	if (out_flags)
		*out_flags = flags;

	return 0;
}

int wgm_peer_array_to_json(const struct wgm_peer_array *peers, json_object **jobj)
{
	json_object *tmp;
	size_t i;
	int ret;

	*jobj = json_object_new_array();
	if (!*jobj)
		return -ENOMEM;

	for (i = 0; i < peers->nr; i++) {
		tmp = json_object_new_object();
		if (!tmp) {
			ret = -ENOMEM;
			goto out;
		}

		ret = wgm_peer_to_json(&peers->peers[i], &tmp);
		if (ret) {
			json_object_put(tmp);
			goto out;
		}

		json_object_array_add(*jobj, tmp);
	}

	ret = 0;
out:
	if (ret) {
		json_object_put(*jobj);
		*jobj = NULL;
	}

	return ret;
}

static char *wgm_iface_to_json_str(const struct wgm_iface *iface)
{
	static const int json_flags = JSON_C_TO_STRING_NOSLASHESCAPE | JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY;
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

	tmp = json_object_new_int(iface->mtu);
	json_object_object_add(jobj, "mtu", tmp);

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

	ret = wgm_peer_array_to_json(&iface->peers, &tmp);
	if (!ret)
		json_object_object_add(jobj, "peers", tmp);
	else
		wgm_log_err("Error: wgm_iface_to_json_str: failed to convert peers to JSON\n");

	jstr = strdup(json_object_to_json_string_ext(jobj, json_flags));
	if (!jstr)
		wgm_log_err("Error: wgm_iface_to_json_str: failed to allocate memory\n");

	json_object_put(jobj);
	return jstr;
}

int wgm_json_to_peer_array(struct wgm_peer_array *peers, const json_object *jobj)
{
	json_object *tmp;
	struct wgm_peer peer;
	size_t i;
	int ret;

	memset(peers, 0, sizeof(*peers));
	peers->nr = json_object_array_length(jobj);
	if (!peers->nr)
		return 0;

	peers->peers = malloc(peers->nr * sizeof(*peers->peers));
	if (!peers->peers)
		return -ENOMEM;

	for (i = 0; i < peers->nr; i++) {
		tmp = json_object_array_get_idx(jobj, i);
		if (!tmp) {
			ret = -EINVAL;
			goto out;
		}

		memset(&peer, 0, sizeof(peer));
		ret = wgm_json_to_peer(&peer, tmp);
		if (ret) {
			wgm_log_err("Error: wgm_iface_json_to_peer_array: failed to convert JSON to peer\n");
			goto out;
		}

		peers->peers[i] = peer;
	}

	return 0;

out:
	while (i > 0)
		wgm_peer_free(&peers->peers[--i]);

	free(peers->peers);
	memset(peers, 0, sizeof(*peers));
	return ret;
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


	tmp = json_object_object_get(jobj, "mtu");
	if (!tmp || !json_object_is_type(tmp, json_type_int)) {
		wgm_log_err("Error: wgm_iface_load_from_json_str: missing or invalid 'mtu' field\n");
		ret = -EINVAL;
		goto out;
	}

	port = json_object_get_int(tmp);
	if (port < 0 || port > 65535) {
		wgm_log_err("Error: wgm_iface_load_from_json_str: invalid 'mtu' field\n");
		ret = -EINVAL;
		goto out;
	}

	iface->mtu = (uint16_t)port;


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


	tmp = json_object_object_get(jobj, "addresses");
	if (tmp) {
		ret = wgm_json_to_str_array(&iface->addresses, tmp);
		if (ret) {
			wgm_log_err("Error: wgm_iface_load_from_json_str: failed to convert 'addresses' to string array\n");
			goto out;
		}
	}


	tmp = json_object_object_get(jobj, "allowed_ips");
	if (tmp) {
		ret = wgm_json_to_str_array(&iface->allowed_ips, tmp);
		if (ret) {
			wgm_log_err("Error: wgm_iface_load_from_json_str: failed to convert 'allowed_ips' to string array\n");
			goto out;
		}
	}


	tmp = json_object_object_get(jobj, "peers");
	if (tmp) {
		ret = wgm_json_to_peer_array(&iface->peers, tmp);
		if (ret) {
			wgm_log_err("Error: wgm_iface_load_from_json_str: failed to convert 'peers' to peer array\n");
			goto out;
		}
	}

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

static char *gen_ipt_bind(const char *iface, const char *ip, const char *src)
{
	static const char fmt[] = "iptables -t nat -A WGM_%s -s %s -j SNAT --to-source %s";
	size_t len;
	char *str;

	len = (size_t)snprintf(NULL, 0, fmt, iface, ip, src);
	str = malloc(len + 1);
	if (!str)
		return NULL;

	snprintf(str, len + 1, fmt, iface, ip, src);
	return str;
}

static int wgm_iface_gen_wg_conf_handle(FILE *fp, const struct wgm_iface *iface)
{
	size_t i, j;

	fprintf(fp, "# Automatically generated by wgm (Wireguard Manager)\n");
	fprintf(fp, "[Interface]\n");
	fprintf(fp, "PrivateKey = %s\n", iface->private_key);
	fprintf(fp, "ListenPort = %u\n", iface->listen_port);
	fprintf(fp, "MTU = %u\n", iface->mtu);
	fprintf(fp, "Table = off\n");

	fprintf(fp, "Address = ");
	for (i = 0; i < iface->addresses.nr; i++) {
		fprintf(fp, "%s", iface->addresses.arr[i]);
		if (i + 1 < iface->addresses.nr)
			fprintf(fp, ", ");
	}

	fprintf(fp, "\n");

	fprintf(fp, "\n");
	fprintf(fp, "PostUp   = iptables -t nat -N WGM_%s || true\n", iface->ifname);
	fprintf(fp, "PostDown = iptables -t nat -D POSTROUTING -s %s ! -d %s -j WGM_%s\n", iface->addresses.arr[0], iface->addresses.arr[0], iface->ifname);
	fprintf(fp, "PostDown = iptables -t nat -F WGM_%s || true\n", iface->ifname);
	fprintf(fp, "PostDown = iptables -t nat -X WGM_%s || true\n", iface->ifname);
	fprintf(fp, "PostUp   = iptables -t filter -I FORWARD -i %s -j ACCEPT\n", iface->ifname);
	fprintf(fp, "PostUp   = iptables -t filter -I FORWARD -o %s -j ACCEPT\n", iface->ifname);

	for (i = 0; i < iface->peers.nr; i++) {
		struct wgm_peer	*peer = &iface->peers.peers[i];
		char *tmp;

		if (peer->bind_ip[0] == '\0')
			continue;

		for (j = 0; j < peer->allowed_ips.nr; j++) {
			tmp = gen_ipt_bind(iface->ifname, peer->allowed_ips.arr[j], peer->bind_ip);
			if (!tmp)
				return -ENOMEM;

			fprintf(fp, "PostUp   = %s\n", tmp);
			free(tmp);
		}
	}

	fprintf(fp, "PostUp   = iptables -t nat -A WGM_%s -j MASQUERADE\n", iface->ifname);
	fprintf(fp, "PostUp   = iptables -t nat -I POSTROUTING -s %s ! -d %s -j WGM_%s\n", iface->addresses.arr[0], iface->addresses.arr[0], iface->ifname);

	for (i = 0; i < iface->peers.nr; i++) {
		const char *bi;

		if (iface->peers.peers[i].bind_ip[0] == '\0')
			bi = "0.0.0.0";
		else
			bi = iface->peers.peers[i].bind_ip;

		fprintf(fp, "\n[Peer]\n");
		fprintf(fp, "# --- BindIP = %s\n", bi);
		fprintf(fp, "PublicKey = %s\n", iface->peers.peers[i].public_key);
		fprintf(fp, "AllowedIPs = ");
		for (j = 0; j < iface->peers.peers[i].allowed_ips.nr; j++) {
			fprintf(fp, "%s", iface->peers.peers[i].allowed_ips.arr[j]);
			if (j + 1 < iface->peers.peers[i].allowed_ips.nr)
				fprintf(fp, ", ");
		}

		fprintf(fp, "\n");
	}

	return 0;
}

static int wgm_iface_gen_wg_conf(struct wgm_ctx *ctx, const struct wgm_iface *iface)
{
	char path[8192];
	FILE *fp;
	int ret;

	ret = snprintf(path, sizeof(path), "%s/%s.conf", ctx->data_dir, iface->ifname);
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

	ret = wgm_iface_gen_wg_conf_handle(fp, iface);
	fclose(fp);
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
	free(jstr);
	return wgm_iface_gen_wg_conf(ctx, iface);
}

int wgm_iface_cmd_del(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t required_args = ARG_IFACE;
	static const uint64_t allowed_args = required_args | ARG_FORCE;
	struct wgm_iface_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	char path[8192];
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));
	ret = wgm_iface_getopt(argc, argv, &arg, allowed_args, required_args, &out_args);
	if (ret < 0)
		return -1;

	ret = wgm_iface_load(ctx, &iface, arg.ifname);
	if (ret == -ENOENT) {
		if (!arg.force) {
			wgm_log_err("Error: wgm_iface_cmd_del: interface does not exist, use -f or --force to silently ignore\n");
			return -ENOENT;
		}

		return 0;
	}

	if (ret) {
		wgm_log_err("Error: wgm_iface_cmd_del: failed to load interface: %s\n", arg.ifname);
		return ret;
	}

	snprintf(path, sizeof(path), "%s/%s.json", ctx->data_dir, arg.ifname);
	ret = remove(path);
	if (ret) {
		ret = -errno;
		wgm_log_err("Error: wgm_iface_cmd_del: failed to remove file: %s: %s\n", path, strerror(-ret));
		return ret;
	}

	return 0;
}

int wgm_iface_cmd_show(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}

static int apply_arg_to_iface(struct wgm_ctx *ctx, struct wgm_iface_arg *arg,
			      struct wgm_iface *iface, uint64_t args)
{

	if (args & ARG_IFACE)
		strncpyl(iface->ifname, arg->ifname, IFNAMSIZ);

	if (args & ARG_LISTEN_PORT)
		iface->listen_port = arg->listen_port;

	if (args & ARG_PRIVATE_KEY)
		strncpyl(iface->private_key, arg->private_key, sizeof(iface->private_key));

	if (args & ARG_MTU)
		iface->mtu = arg->mtu;

	if (args & ARG_ADDRESS)
		iface->addresses = arg->addresses;

	if (args & ARG_ALLOWED_IP)
		iface->allowed_ips = arg->allowed_ips;

	wgm_iface_dump_json(iface);
	return wgm_iface_save(ctx, iface);
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
int wgm_iface_cmd_add(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t required_args = ARG_IFACE | ARG_LISTEN_PORT | ARG_PRIVATE_KEY | ARG_ADDRESS | ARG_ALLOWED_IP | ARG_MTU;
	static const uint64_t allowed_args = required_args | ARG_FORCE;
	struct wgm_iface_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));
	ret = wgm_iface_getopt(argc, argv, &arg, allowed_args, required_args, &out_args);
	if (ret < 0)
		return -1;

	ret = wgm_iface_load(ctx, &iface, arg.ifname);
	if (!ret) {
		if (!arg.force) {
			wgm_log_err("Error: wgm_iface_cmd_add: interface already exists, use -f or --force to force update\n");
			return -EEXIST;
		}

		printf("# Force flag is set and interface already exists, updating...\n");
		goto out;
	}

	if (ret != -ENOENT) {
		if (!arg.force)
			return ret;

		printf("# Force flag is set, ignoring error...\n");
	}

out:
	ret = apply_arg_to_iface(ctx, &arg, &iface, out_args);
	if (ret) {
		wgm_log_err("Error: wgm_iface_cmd_add: failed to save interface: %s: %s\n", arg.ifname, strerror(-ret));
		return ret;
	}

	wgm_iface_free(&iface);
	return 0;
}

int wgm_iface_cmd_update(int argc, char *argv[], struct wgm_ctx *ctx)
{
	static const uint64_t required_args = ARG_IFACE;
	static const uint64_t allowed_args = required_args | ARG_LISTEN_PORT | ARG_PRIVATE_KEY | ARG_FORCE;
	struct wgm_iface_arg arg;
	struct wgm_iface iface;
	uint64_t out_args = 0;
	int ret;

	memset(&arg, 0, sizeof(arg));
	memset(&iface, 0, sizeof(iface));
	ret = wgm_iface_getopt(argc, argv, &arg, allowed_args, required_args, &out_args);
	if (ret < 0)
		return -1;

	ret = wgm_iface_load(ctx, &iface, arg.ifname);
	if (ret) {
		wgm_log_err("Error: wgm_iface_cmd_update: failed to load interface: %s: %s\n", arg.ifname, strerror(-ret));
		return ret;
	}

	ret = apply_arg_to_iface(ctx, &arg, &iface, out_args);
	if (ret) {
		wgm_log_err("Error: wgm_iface_cmd_update: failed to save interface: %s: %s\n", arg.ifname, strerror(-ret));
		return ret;
	}

	wgm_iface_free(&iface);
	return 0;
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
		printf("    Peer %zu:\n", i);
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

	/*
	 * If the peer already exists, return -EEXIST.
	 */
	for (i = 0; i < iface->peers.nr; i++) {
		if (strcmp(iface->peers.peers[i].public_key, peer->public_key) == 0)
			return -EEXIST;
	}

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

void wgm_iface_dump_json(const struct wgm_iface *iface)
{
	char *jstr = wgm_iface_to_json_str(iface);
	if (!jstr) {
		wgm_log_err("Error: wgm_iface_dump_json: failed to convert interface to JSON\n");
		return;
	}

	printf("%s\n", jstr);
}
