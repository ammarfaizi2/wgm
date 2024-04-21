// SPDX-License-Identifier: GPL-2.0-only

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#include "wgm_cmd_iface.h"

#define WGM_IFACE_DEF_MTU	1400u
#define WGM_IFACE_DEF_PORT	51820u

static const struct wgm_opt options[] = {
	#define IFACE_OPT_DEV		(1ull << 0ull)
	{ IFACE_OPT_DEV,		"dev",		required_argument,	NULL,	'd' },

	#define IFACE_OPT_PRIVATE_KEY	(1ull << 1ull)
	{ IFACE_OPT_PRIVATE_KEY,	"private-key",	required_argument,	NULL,	'k' },

	#define IFACE_OPT_LISTEN_PORT	(1ull << 2ull)
	{ IFACE_OPT_LISTEN_PORT,	"listen-port",	required_argument,	NULL,	'p' },

	#define IFACE_OPT_ADDRS		(1ull << 3ull)
	{ IFACE_OPT_ADDRS,		"addrs",	required_argument,	NULL,	'A' },

	#define IFACE_OPT_MTU		(1ull << 4ull)
	{ IFACE_OPT_MTU,		"mtu",		required_argument,	NULL,	'm' },

	#define IFACE_OPT_FORCE		(1ull << 5ull)
	{ IFACE_OPT_FORCE,		"force",	no_argument,		NULL,	'f' },

	#define IFACE_OPT_UP		(1ull << 6ull)
	{ IFACE_OPT_UP,			"up",		no_argument,		NULL,	'u' },

	{ 0, NULL, 0, NULL, 0 }
};

struct wgm_iface_arg {
	const char		*dev;
	const char		*private_key;
	uint16_t		listen_port;
	struct wgm_array_str	addrs;
	uint16_t		mtu;
	bool			force;
	bool			up;
};

static void wgm_iface_arg_free(struct wgm_iface_arg *arg)
{
	wgm_array_str_free(&arg->addrs);
	memset(arg, 0, sizeof(*arg));
}

void wgm_cmd_iface_show_usage(const char *app, int show_cmds)
{
	printf("Usage: %s iface [list|show|add|del|update|up|down] <options>\n", app);
	if (show_cmds) {
		printf("\nCommands:\n");
		printf("  list                             List all interfaces\n");
		printf("  show <ifname>                    Show interface details\n");
		printf("  add <options>                    Add a new interface\n");
		printf("  update <options>                 Update an interface\n");
		printf("  del <ifname>                     Delete an interface\n");
		printf("  up <ifname>                      Start an interface\n");
		printf("  down <ifname>                    Stop an interface\n");
	}

	printf("\nOptions:\n");
	printf("  -d, --dev <ifname>               Set the interface name\n");
	printf("  -k, --private-key <key>          Set the private key\n");
	printf("  -p, --listen-port <port>         Set the listen port (default: %hu)\n", WGM_IFACE_DEF_PORT);
	printf("  -A, --addrs <addrs>              Set the interface addresses (comma separated list)\n");
	printf("  -m, --mtu <mtu>                  Set the interface MTU (default: %hu)\n", WGM_IFACE_DEF_MTU);
	printf("  -f, --force                      Force the operation\n");
	printf("  -u, --up                         Start the interface after adding/updating\n");
	printf("\n");
}

static int parse_args(int argc, char *argv[], struct wgm_iface_arg *arg,
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
			out_args |= IFACE_OPT_DEV;
			break;
		
		case 'k':
			arg->private_key = optarg;
			out_args |= IFACE_OPT_PRIVATE_KEY;
			break;
		
		case 'p':
			arg->listen_port = atoi(optarg);
			out_args |= IFACE_OPT_LISTEN_PORT;
			break;
		
		case 'A':
			wgm_array_str_free(&arg->addrs);
			ret = wgm_array_str_from_csv(&arg->addrs, optarg);
			if (ret)
				return ret;

			out_args |= IFACE_OPT_ADDRS;
			break;

		case 'm':
			arg->mtu = atoi(optarg);
			out_args |= IFACE_OPT_MTU;
			break;
		
		case 'f':
			arg->force = true;
			out_args |= IFACE_OPT_FORCE;
			break;
		
		case 'u':
			arg->up = true;
			out_args |= IFACE_OPT_UP;
			break;
		
		case '?':
			printf("Invalid option: %s\n", argv[optind - 1]);
			ret = -EINVAL;
			goto out;
		default:
			printf("Unknown option: %c\n", c);
			ret = -EINVAL;
			goto out;
		}
	}

out:
	wgm_free_getopt_long_args(long_opt, short_opt);
	*out_args_p = out_args;
	return ret;
}

static bool validate_args(const char *app, struct wgm_iface_arg *arg,
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
			wgm_cmd_iface_show_usage(app, 1);
			return false;
		}

		if (!(id & allowed_bits) && (id & arg_bits)) {
			wgm_err_elog_add("Option --%s is not allowed for this command.\n", options[i].name);
			wgm_cmd_iface_show_usage(app, 1);
			return false;
		}
	}

	return true;
}

static int wgm_cmd_iface_list(const char *app, struct wgm_ctx *ctx,
			      struct wgm_iface_arg *arg, uint64_t arg_bits)
{
	return 0;
}

static int wgm_cmd_iface_show(const char *app, struct wgm_ctx *ctx,
			      struct wgm_iface_arg *arg, uint64_t arg_bits)
{
	return 0;
}

static void wg_apply_def_to_iface(struct wgm_iface *iface)
{
	memset(iface, 0, sizeof(*iface));
	iface->listen_port = WGM_IFACE_DEF_PORT;
	iface->mtu = WGM_IFACE_DEF_MTU;
}

static int wg_apply_arg_to_iface(struct wgm_iface *iface, struct wgm_iface_arg *arg,
				 uint64_t arg_bits)
{
	if (arg_bits & IFACE_OPT_DEV)
		strncpy(iface->ifname, arg->dev, sizeof(iface->ifname));

	if (arg_bits & IFACE_OPT_PRIVATE_KEY)
		strncpy(iface->private_key, arg->private_key, sizeof(iface->private_key));

	if (arg_bits & IFACE_OPT_LISTEN_PORT)
		iface->listen_port = arg->listen_port;

	if (arg_bits & IFACE_OPT_ADDRS)
		wgm_array_str_move(&iface->addresses, &arg->addrs);

	if (arg_bits & IFACE_OPT_MTU)
		iface->mtu = arg->mtu;

	return 0;
}

static int wg_cmd_iface_add(const char *app, struct wgm_ctx *ctx,
			    struct wgm_iface_arg *arg, uint64_t arg_bits)
{
	static const uint64_t required_bits = IFACE_OPT_DEV | IFACE_OPT_PRIVATE_KEY;
	static const uint64_t allowed_bits = IFACE_OPT_DEV | IFACE_OPT_PRIVATE_KEY |
					     IFACE_OPT_LISTEN_PORT | IFACE_OPT_ADDRS |
					     IFACE_OPT_MTU | IFACE_OPT_FORCE | IFACE_OPT_UP;

	struct wgm_iface_hdl hdl;
	struct wgm_iface iface;
	int ret = 0;

	if (!validate_args(app, arg, arg_bits, required_bits, allowed_bits))
		return -EINVAL;

	wg_apply_def_to_iface(&iface);

	hdl.ctx = ctx;
	ret = wgm_iface_hdl_open(&hdl, arg->dev, true);
	if (ret) {
		wgm_err_elog_add("Failed to open interface file: %s: %s\n", arg->dev, strerror(-ret));
		return ret;
	}

	ret = wgm_iface_hdl_load(&hdl, &iface);
	if (ret && ret != -ENODEV) {
		wgm_err_elog_add("Failed to load interface file: %s: %s\n", arg->dev, strerror(-ret));
		if (!arg->force) {
			wgm_err_elog_add("Use --force to overwrite\n");
			goto out;
		}
	}

	if (!ret && !arg->force) {
		wgm_err_elog_add("Interface %s already exists. Use --force to update it.\n", arg->dev);
		goto out;
	}

	ret = wg_apply_arg_to_iface(&iface, arg, arg_bits);
	if (ret) {
		wgm_err_elog_add("Failed to apply arguments to interface: %s\n", strerror(-ret));
		goto out;
	}

	ret = wgm_iface_hdl_store(&hdl, &iface);
	if (ret) {
		wgm_err_elog_add("Failed to store interface: %s: %s\n", arg->dev, strerror(-ret));
		goto out;
	}

out:
	wgm_iface_hdl_close(&hdl);
	wgm_iface_free(&iface);
	return ret;
}

static int wg_cmd_iface_del(const char *app, struct wgm_ctx *ctx,
			    struct wgm_iface_arg *arg, uint64_t arg_bits)
{
	return 0;
}

static int wg_cmd_iface_update(const char *app, struct wgm_ctx *ctx,
			       struct wgm_iface_arg *arg, uint64_t arg_bits)
{
	const uint64_t required_bits = IFACE_OPT_DEV;
	const uint64_t allowed_bits = IFACE_OPT_DEV | IFACE_OPT_PRIVATE_KEY |
				      IFACE_OPT_LISTEN_PORT | IFACE_OPT_ADDRS |
				      IFACE_OPT_MTU | IFACE_OPT_FORCE | IFACE_OPT_UP;

	struct wgm_iface_hdl hdl;
	struct wgm_iface iface;
	int ret;

	if (!validate_args(app, arg, arg_bits, required_bits, allowed_bits))
		return -EINVAL;

	hdl.ctx = ctx;
	ret = wgm_iface_hdl_open(&hdl, arg->dev, false);
	if (ret) {
		wgm_err_elog_add("Failed to open interface file: %s: %s\n", arg->dev, strerror(-ret));
		return ret;
	}

	ret = wgm_iface_hdl_load(&hdl, &iface);
	if (ret) {
		wgm_err_elog_add("Failed to load interface file: %s: %s\n", arg->dev, strerror(-ret));
		goto out;
	}

	ret = wg_apply_arg_to_iface(&iface, arg, arg_bits);
	if (ret) {
		wgm_err_elog_add("Failed to apply arguments to interface: %s\n", strerror(-ret));
		goto out;
	}

	ret = wgm_iface_hdl_store(&hdl, &iface);
	if (ret) {
		wgm_err_elog_add("Failed to store interface: %s: %s\n", arg->dev, strerror(-ret));
		goto out;
	}

out:
	wgm_iface_hdl_close(&hdl);
	wgm_iface_free(&iface);
	return ret;
}

static int wgm_cmd_iface_run(const char *app, struct wgm_ctx *ctx, const char *cmd,
			     struct wgm_iface_arg *arg, uint64_t arg_bits)
{
	if (!strcmp(cmd, "list"))
		return wgm_cmd_iface_list(app, ctx, arg, arg_bits);

	if (!strcmp(cmd, "show"))
		return wgm_cmd_iface_show(app, ctx, arg, arg_bits);

	if (!strcmp(cmd, "add"))
		return wg_cmd_iface_add(app, ctx, arg, arg_bits);

	if (!strcmp(cmd, "del"))
		return wg_cmd_iface_del(app, ctx, arg, arg_bits);

	if (!strcmp(cmd, "update"))
		return wg_cmd_iface_update(app, ctx, arg, arg_bits);

	return -EINVAL;
}

int wgm_cmd_iface(int argc, char *argv[], struct wgm_ctx *ctx)
{
	struct wgm_iface_arg arg;
	const char *cmd, *app;
	uint64_t out_args = 0;
	int ret;

	if (argc < 2) {
		wgm_cmd_iface_show_usage(argv[0], 1);
		return -EINVAL;
	}

	app = argv[0];
	cmd = argv[2];
	ret = parse_args(argc, argv, &arg, &out_args);
	if (ret)
		return ret;

	ret = wgm_cmd_iface_run(app, ctx, cmd, &arg, out_args);
	wgm_iface_arg_free(&arg);
	return ret;
}

int wgm_iface_to_json(const struct wgm_iface *iface, json_object **obj)
{
	json_object *ret, *tmp;
	int err = 0;

	ret = json_object_new_object();
	if (!ret)
		return -ENOMEM;

	err |= json_object_object_add(ret, "ifname", json_object_new_string(iface->ifname));
	err |= json_object_object_add(ret, "private_key", json_object_new_string(iface->private_key));
	err |= json_object_object_add(ret, "listen_port", json_object_new_int(iface->listen_port));
	if (err)
		goto out_err;

	err = wgm_array_str_to_json(&iface->addresses, &tmp);
	if (err)
		goto out_err;

	err = json_object_object_add(ret, "addresses", tmp);
	if (err)
		goto out_err;

	err = wgm_array_peer_to_json(&iface->peers, &tmp);
	if (err)
		goto out_err;

	err = json_object_object_add(ret, "peers", tmp);
	if (err)
		goto out_err;

	*obj = ret;
	return 0;

out_err:
	json_object_put(ret);
	return -ENOMEM;
}

int wgm_iface_from_json(struct wgm_iface *iface, const json_object *obj)
{
	return 0;
}

int wgm_iface_copy(struct wgm_iface *dst, const struct wgm_iface *src)
{
	struct wgm_iface tmp;

	tmp = *src;

	if (wgm_array_str_copy(&tmp.addresses, &src->addresses))
		return -ENOMEM;

	if (wgm_array_peer_copy(&tmp.peers, &src->peers)) {
		wgm_array_str_free(&tmp.addresses);
		return -ENOMEM;
	}

	*dst = tmp;
	return 0;
}

void wgm_iface_move(struct wgm_iface *dst, struct wgm_iface *src)
{
	*dst = *src;
	wgm_array_str_move(&dst->addresses, &src->addresses);
	wgm_array_peer_move(&dst->peers, &src->peers);
	wgm_iface_free(src);
}

void wgm_iface_free(struct wgm_iface *iface)
{
	wgm_array_str_free(&iface->addresses);
	wgm_array_peer_free(&iface->peers);
	memset(iface, 0, sizeof(*iface));
}

static int wgm_iface_gen_path(char **path, struct wgm_ctx *ctx,
				const char *dev, const char *fmt, ...)
{
	va_list ap1, ap2;
	char *rpath;
	size_t len;
	size_t off;
	int ret;

	va_start(ap1, fmt);
	va_copy(ap2, ap1);
	len = (size_t)vsnprintf(NULL, 0, fmt, ap1);
	va_end(ap1);

	len += (size_t)snprintf(NULL, 0, "%s/%s/", ctx->data_dir, dev);
	rpath = malloc(len + 1);
	if (!rpath)
		return -ENOMEM;

	off = (size_t)snprintf(rpath, len + 1, "%s/%s/", ctx->data_dir, dev);
	ret = wgm_mkdir_recursive(rpath, 0700);
	if (ret)
		goto out;

	vsnprintf(rpath + off, len - off + 1, fmt, ap2);
	ret = 0;

out:
	if (ret)
		free(rpath);
	else
		*path = rpath;

	return ret;
}

int wgm_iface_hdl_open(struct wgm_iface_hdl *hdl, const char *dev, bool create_new)
{
	char *path;
	int ret;

	ret = wgm_iface_gen_path(&path, hdl->ctx, dev, "iface.json");
	if (ret)
		return ret;

	ret = wgm_file_open_lock(&hdl->file, path, "rb+", LOCK_EX);
	if (ret == -ENOENT && create_new)
		ret = wgm_file_open_lock(&hdl->file, path, "wb+", LOCK_EX);

	if (!ret)
		fseek(hdl->file.file, 0, SEEK_SET);

	free(path);
	return ret;
}

int wgm_iface_hdl_close(struct wgm_iface_hdl *hdl)
{
	return wgm_file_close(&hdl->file);
}

int wgm_iface_hdl_load(struct wgm_iface_hdl *hdl, struct wgm_iface *iface)
{
	json_object *obj;
	char *jstr;
	size_t len;
	int ret;

	ret = wgm_file_get_contents(&hdl->file, &jstr, &len);
	fseek(hdl->file.file, 0, SEEK_SET);
	if (ret)
		return ret;

	if (!len) {
		free(jstr);
		return -ENODEV;
	}

	obj = json_tokener_parse(jstr);
	if (!obj) {
		ret = -EINVAL;
		goto out;
	}

	ret = wgm_iface_from_json(iface, obj);
	json_object_put(obj);

out:
	free(jstr);
	return ret;
}

int wgm_iface_hdl_store(struct wgm_iface_hdl *hdl, struct wgm_iface *iface)
{
	json_object *obj;
	const char *jstr;
	int ret;

	ret = wgm_iface_to_json(iface, &obj);
	if (ret)
		return ret;

	jstr = json_object_to_json_string_ext(obj, WGM_JSON_FLAGS);
	if (!jstr) {
		json_object_put(obj);
		return -ENOMEM;
	}

	ret = wgm_file_put_contents(&hdl->file, jstr, strlen(jstr));
	json_object_put(obj);
	return ret;
}
