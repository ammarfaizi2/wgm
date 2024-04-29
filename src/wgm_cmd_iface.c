// SPDX-License-Identifier: GPL-2.0-only

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

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

	#define IFACE_OPT_FORCE		(1ull << 62ull)
	{ IFACE_OPT_FORCE,		"force",	no_argument,		NULL,	'f' },

	#define IFACE_OPT_UP		(1ull << 63ull)
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

static int wgm_iface_path_fmt(bool create_dir, char **path, struct wgm_ctx *ctx,
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
	if (create_dir) {
		ret = wgm_mkdir_recursive(rpath, 0700);
		if (ret)
			goto out;
	}

	vsnprintf(rpath + off, len - off + 1, fmt, ap2);
	ret = 0;

out:
	if (ret)
		free(rpath);
	else
		*path = rpath;

	return ret;
}

void wgm_cmd_iface_show_usage(const char *app, int show_cmds)
{
	printf("Usage: %s iface [list|show|add|del|update|up|down] <arguments>\n", app);
	if (show_cmds) {
		printf("\nCommands:\n\n");
		printf("  add    <options>           Add a new interface\n");
		printf("  update <options>           Update an interface\n");
		printf("  show   <ifname>            Show an interface\n");
		printf("  del    <ifname>            Delete an interface\n");
		printf("  list                       List all interfaces\n");
		printf("  up     <ifname>            Start an interface\n");
		printf("  down   <ifname>            Stop an interface\n");
	}

	printf("\nOptions (for 'add' and 'update' only):\n\n");
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
				goto out;

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
	struct wgm_array_iface ifarr;
	struct wgm_iface_hdl hdl;
	wgm_global_lock_t glock;
	json_object *ifarr_json;
	const char *ifarr_jstr;
	int err = 0;
	DIR *dir;

	if (arg_bits) {
		wgm_err_elog_add("Error: The list command does not take any options.\n");
		wgm_cmd_iface_show_usage(app, 1);
		return -EINVAL;
	}

	dir = opendir(ctx->data_dir);
	if (!dir) {
		err = -errno;
		wgm_err_elog_add("Failed to open data directory: %s: %s\n", ctx->data_dir, strerror(-err));
		return err;
	}

	err = wgm_global_lock_open(&glock, ctx, "iface.lock");
	if (err) {
		closedir(dir);
		return err;
	}

	memset(&ifarr, 0, sizeof(ifarr));
	while (1) {
		struct wgm_iface iface;
		struct dirent *ent;

		ent = readdir(dir);
		if (!ent)
			break;

		if (ent->d_name[0] == '.')
			continue;

		if (ent->d_type != DT_DIR)
			continue;

		hdl.ctx = ctx;
		err = __wgm_iface_hdl_open(&hdl, ent->d_name, false);
		if (err)
			continue;

		memset(&iface, 0, sizeof(iface));
		err = wgm_iface_hdl_load(&hdl, &iface);
		wgm_iface_hdl_close(&hdl);
		if (err) {
			wgm_iface_hdl_close(&hdl);
			continue;
		}

		err = wgm_array_iface_add_mv(&ifarr, &iface);
		if (err) {
			wgm_iface_free(&iface);
			continue;
		}
	}

	if (err)
		goto out;

	err = wgm_array_iface_to_json(&ifarr, &ifarr_json);
	if (err) {
		wgm_err_elog_add("Failed to convert interface array to JSON: %s\n", strerror(-err));
		goto out;
	}

	ifarr_jstr = json_object_to_json_string_ext(ifarr_json, WGM_JSON_FLAGS);
	if (!ifarr_jstr) {
		json_object_put(ifarr_json);
		wgm_err_elog_add("Failed to convert JSON object to string\n");
		err = -ENOMEM;
		goto out;
	}

	puts(ifarr_jstr);
	json_object_put(ifarr_json);

out:
	wgm_array_iface_free(&ifarr);
	wgm_global_lock_close(&glock);
	closedir(dir);
	return err;
}

static int wgm_cmd_iface_show(const char *app, struct wgm_ctx *ctx,
			      struct wgm_iface_arg *arg, uint64_t arg_bits,
			      const char *first_arg)
{
	const char *dev = first_arg;
	struct wgm_iface_hdl hdl;
	struct wgm_iface iface;
	const char *ifjson;
	json_object *ifobj;
	int ret;

	if (arg_bits) {
		wgm_err_elog_add("Error: The show command does not take any options.\n");
		wgm_err_elog_add("Use 'show <ifname>' to show an interface details.\n");
		wgm_cmd_iface_show_usage(app, 1);
		return -EINVAL;
	}

	hdl.ctx = ctx;
	ret = wgm_iface_hdl_open(&hdl, dev, false);
	if (ret) {
		wgm_err_elog_add("Failed to open interface file: %s: %s\n", dev, strerror(-ret));
		return ret;
	}

	memset(&iface, 0, sizeof(iface));
	ret = wgm_iface_hdl_load(&hdl, &iface);
	if (ret) {
		wgm_err_elog_add("Failed to load interface file: %s: %s\n", dev, strerror(-ret));
		goto out;
	}

	ret = wgm_iface_to_json(&iface, &ifobj);
	if (ret) {
		wgm_err_elog_add("Failed to convert interface to JSON: %s\n", strerror(-ret));
		goto out;
	}

	ifjson = json_object_to_json_string_ext(ifobj, WGM_JSON_FLAGS);
	if (!ifjson) {
		json_object_put(ifobj);
		wgm_err_elog_add("Failed to convert JSON object to string\n");
		ret = -ENOMEM;
		goto out;
	}
	puts(ifjson);
	json_object_put(ifobj);

out:
	wgm_iface_hdl_close(&hdl);
	wgm_iface_free(&iface);
	return ret;
}

static int wg_cmd_iface_del(const char *app, struct wgm_ctx *ctx,
			    struct wgm_iface_arg *arg, uint64_t arg_bits,
			    const char *first_arg)
{
	const char *dev = first_arg;
	struct wgm_iface_hdl hdl;
	struct wgm_iface iface;
	char *file_path;
	int ret;

	if (arg_bits) {
		wgm_err_elog_add("Error: The del command does not take any options.\n");
		wgm_err_elog_add("Use 'del <ifname>' to delete an interface.\n");
		wgm_cmd_iface_show_usage(app, 1);
		return -EINVAL;
	}

	hdl.ctx = ctx;
	ret = wgm_iface_hdl_open(&hdl, dev, false);
	if (ret) {
		wgm_err_elog_add("Failed to open interface file: %s: %s\n", dev, strerror(-ret));
		return ret;
	}

	memset(&iface, 0, sizeof(iface));
	ret = wgm_iface_hdl_load(&hdl, &iface);
	if (ret) {
		wgm_err_elog_add("Failed to load interface file: %s: %s\n", dev, strerror(-ret));
		goto out;
	}

	ret = wgm_iface_path_fmt(false, &file_path, ctx, dev, "iface.json");
	if (ret) {
		wgm_err_elog_add("Failed to format interface file path: %s\n", strerror(-ret));
		goto out;
	}

	ret = remove(file_path);
	if (ret) {
		ret = -errno;
		wgm_err_elog_add("Failed to remove interface file: %s: %s\n", file_path, strerror(-ret));
		free(file_path);
		goto out;
	}

	free(file_path);

	ret = wgm_iface_path_fmt(false, &file_path, ctx, dev, "");
	if (ret) {
		wgm_err_elog_add("Failed to format interface directory path: %s\n", strerror(-ret));
		goto out;
	}

	ret = rmdir(file_path);
	if (ret) {
		ret = -errno;
		wgm_err_elog_add("Failed to remove interface directory: %s: %s\n", file_path, strerror(-ret));
		free(file_path);
		goto out;
	}

	free(file_path);

out:
	wgm_iface_hdl_close(&hdl);
	wgm_iface_free(&iface);
	return ret;
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
		strncpyl(iface->ifname, arg->dev, sizeof(iface->ifname));

	if (arg_bits & IFACE_OPT_PRIVATE_KEY)
		strncpyl(iface->private_key, arg->private_key, sizeof(iface->private_key));

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
			     struct wgm_iface_arg *arg, uint64_t arg_bits,
			     const char *first_arg)
{
	if (!strcmp(cmd, "list"))
		return wgm_cmd_iface_list(app, ctx, arg, arg_bits);

	if (!strcmp(cmd, "show"))
		return wgm_cmd_iface_show(app, ctx, arg, arg_bits, first_arg);

	if (!strcmp(cmd, "add"))
		return wg_cmd_iface_add(app, ctx, arg, arg_bits);

	if (!strcmp(cmd, "del"))
		return wg_cmd_iface_del(app, ctx, arg, arg_bits, first_arg);

	if (!strcmp(cmd, "update"))
		return wg_cmd_iface_update(app, ctx, arg, arg_bits);

	if (!strcmp(cmd, "up"))
		return -ENOSYS;

	if (!strcmp(cmd, "down"))
		return -ENOSYS;

	wgm_err_elog_add("Error: Unknown command: '%s'\n", cmd);
	wgm_cmd_iface_show_usage(app, 1);
	return -EINVAL;
}

int wgm_cmd_iface(int argc, char *argv[], struct wgm_ctx *ctx)
{
	const char *cmd, *app, *first_arg;
	struct wgm_iface_arg arg;
	uint64_t out_args = 0;
	int ret;

	if (argc < 3) {
		wgm_cmd_iface_show_usage(argv[0], 1);
		return -EINVAL;
	}

	app = argv[0];
	cmd = argv[2];
	first_arg = argv[3];
	ret = parse_args(argc, argv, &arg, &out_args);
	if (ret)
		return ret;

	ret = wgm_cmd_iface_run(app, ctx, cmd, &arg, out_args, first_arg);
	wgm_iface_arg_free(&arg);
	return ret;
}

int wgm_iface_to_json(const struct wgm_iface *iface, json_object **obj)
{
	json_object *ret;
	int err = 0;

	ret = json_object_new_object();
	if (!ret)
		return -ENOMEM;

	err |= wgm_json_obj_set_str(ret, "ifname", iface->ifname);
	err |= wgm_json_obj_set_str(ret, "private_key", iface->private_key);
	err |= wgm_json_obj_set_uint16_t(ret, "listen_port", iface->listen_port);
	err |= wgm_json_obj_set_uint16_t(ret, "mtu", iface->mtu);
	err |= wgm_json_obj_set_str_array(ret, "addresses", &iface->addresses);
	err |= wgm_json_obj_set_peer_array(ret, "peers", &iface->peers);
	if (err) {
		json_object_put(ret);
		return -ENOMEM;
	}

	*obj = ret;
	return 0;
}

int wgm_iface_from_json(struct wgm_iface *iface, const json_object *obj)
{
	int err = 0;

	err |= wgm_json_obj_kcp_str(obj, "ifname", iface->ifname, sizeof(iface->ifname));
	err |= wgm_json_obj_kcp_str(obj, "private_key", iface->private_key, sizeof(iface->private_key));
	err |= wgm_json_obj_kcp_uint16_t(obj, "listen_port", &iface->listen_port);
	err |= wgm_json_obj_kcp_uint16_t(obj, "mtu", &iface->mtu);
	if (err)
		return -EINVAL;

	err = wgm_json_obj_kcp_str_array(obj, "addresses", &iface->addresses);
	if (err)
		return err;

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

int __wgm_iface_hdl_open(struct wgm_iface_hdl *hdl, const char *dev, bool create_new)
{
	char *path;
	int ret;

	ret = wgm_iface_path_fmt(create_new, &path, hdl->ctx, dev, "iface.json");
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

int wgm_iface_hdl_open(struct wgm_iface_hdl *hdl, const char *dev, bool create_new)
{
	wgm_global_lock_t lock;
	int ret;

	ret = wgm_global_lock_open(&lock, hdl->ctx, "iface.lock");
	if (ret)
		return ret;

	ret = __wgm_iface_hdl_open(hdl, dev, create_new);
	wgm_global_lock_close(&lock);
	return ret;
}

int wgm_iface_hdl_open_and_load(struct wgm_iface_hdl *hdl, const char *dev,
				bool create_new, struct wgm_iface *iface)
{
	int ret;

	ret = wgm_iface_hdl_open(hdl, dev, create_new);
	if (ret) {
		wgm_err_elog_add("Failed to open interface file: %s: %s\n", dev, strerror(-ret));
		return ret;
	}

	ret = wgm_iface_hdl_load(hdl, iface);
	if (ret) {
		if (!create_new)
			wgm_err_elog_add("Failed to load interface file: %s: %s\n", dev, strerror(-ret));

		wgm_iface_hdl_close(hdl);
	}

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

int wgm_array_iface_add(struct wgm_array_iface *arr, const struct wgm_iface *iface)
{
	struct wgm_iface *tmp;
	size_t new_len;
	int ret;

	new_len = arr->len + 1;
	tmp = realloc(arr->arr, new_len * sizeof(*tmp));
	if (!tmp)
		return -ENOMEM;

	ret = wgm_iface_copy(&tmp[arr->len], iface);
	if (ret)
		return ret;

	arr->arr = tmp;
	arr->len = new_len;
	return 0;
}

int wgm_array_iface_add_mv(struct wgm_array_iface *arr, struct wgm_iface *iface)
{
	struct wgm_iface *tmp;
	size_t new_len;

	new_len = arr->len + 1;
	tmp = realloc(arr->arr, new_len * sizeof(*tmp));
	if (!tmp)
		return -ENOMEM;

	tmp[arr->len] = *iface;
	memset(iface, 0, sizeof(*iface));
	arr->arr = tmp;
	arr->len = new_len;
	return 0;
}

int wgm_array_iface_del(struct wgm_array_iface *arr, size_t idx)
{
	size_t i;

	if (idx >= arr->len)
		return -EINVAL;

	wgm_iface_free(&arr->arr[idx]);
	for (i = idx; i < arr->len - 1; i++)
		arr->arr[i] = arr->arr[i + 1];

	arr->len--;
	return 0;
}

int wgm_array_iface_find(struct wgm_array_iface *arr, const char *ifname, size_t *idx)
{
	size_t i;

	for (i = 0; i < arr->len; i++) {
		if (!strcmp(arr->arr[i].ifname, ifname)) {
			*idx = i;
			return 0;
		}
	}

	return -ENOENT;
}

int wgm_array_iface_add_unique(struct wgm_array_iface *arr, const struct wgm_iface *iface)
{
	size_t idx;
	int ret;

	ret = wgm_array_iface_find(arr, iface->ifname, &idx);
	if (!ret)
		return 0;

	if (ret != -ENOENT)
		return ret;

	return wgm_array_iface_add(arr, iface);
}

int wgm_array_iface_copy(struct wgm_array_iface *dst, const struct wgm_array_iface *src)
{
	size_t i;
	int ret;

	memset(dst, 0, sizeof(*dst));
	for (i = 0; i < src->len; i++) {
		ret = wgm_array_iface_add(dst, &src->arr[i]);
		if (ret)
			return ret;
	}

	return 0;
}

void wgm_array_iface_free(struct wgm_array_iface *arr)
{
	size_t i;

	for (i = 0; i < arr->len; i++)
		wgm_iface_free(&arr->arr[i]);

	free(arr->arr);
	memset(arr, 0, sizeof(*arr));
}

void wgm_array_iface_move(struct wgm_array_iface *dst, struct wgm_array_iface *src)
{
	*dst = *src;
	memset(src, 0, sizeof(*src));
}

int wgm_array_iface_to_json(const struct wgm_array_iface *arr, json_object **obj)
{
	json_object *ret, *tmp;
	size_t i;
	int err = 0;

	ret = json_object_new_array();
	if (!ret)
		return -ENOMEM;

	for (i = 0; i < arr->len; i++) {
		err = wgm_iface_to_json(&arr->arr[i], &tmp);
		if (err)
			goto out_err;

		err = json_object_array_add(ret, tmp);
		if (err) {
			json_object_put(tmp);
			goto out_err;
		}
	}

	*obj = ret;
	return 0;

out_err:
	json_object_put(ret);
	return -ENOMEM;
}

int wgm_array_iface_from_json(struct wgm_array_iface *arr, const json_object *obj)
{
	size_t i;
	int ret;

	if (!json_object_is_type(obj, json_type_array))
		return -EINVAL;

	memset(arr, 0, sizeof(*arr));
	for (i = 0; i < (size_t)json_object_array_length(obj); i++) {
		struct wgm_iface iface;
		json_object *tmp;

		tmp = json_object_array_get_idx(obj, i);
		if (!tmp) {
			wgm_array_iface_free(arr);
			return -EINVAL;
		}

		ret = wgm_iface_from_json(&iface, tmp);
		if (ret) {
			wgm_array_iface_free(arr);
			return ret;
		}

		ret = wgm_array_iface_add(arr, &iface);
		wgm_iface_free(&iface);
		if (ret) {
			wgm_array_iface_free(arr);
			return ret;
		}
	}

	return 0;
}

int wgm_iface_peer_add(struct wgm_iface *iface, const struct wgm_peer *peer)
{
	struct wgm_peer *tmp;
	size_t new_len;
	int ret;

	new_len = iface->peers.len + 1;
	tmp = realloc(iface->peers.arr, new_len * sizeof(*tmp));
	if (!tmp)
		return -ENOMEM;

	iface->peers.arr = tmp;
	ret = wgm_peer_copy(&tmp[iface->peers.len], peer);
	if (ret)
		return ret;

	iface->peers.len = new_len;
	return 0;
}

int wgm_iface_peer_del(struct wgm_iface *iface, size_t idx)
{
	if (idx >= iface->peers.len)
		return -EINVAL;

	wgm_peer_free(&iface->peers.arr[idx]);
	for (size_t i = idx; i < iface->peers.len - 1; i++)
		iface->peers.arr[i] = iface->peers.arr[i + 1];

	iface->peers.len--;
	return 0;
}

int wgm_iface_peer_find(struct wgm_iface *iface, const char *pub_key, size_t *idx)
{
	size_t i;

	for (i = 0; i < iface->peers.len; i++) {
		if (!strcmp(iface->peers.arr[i].public_key, pub_key)) {
			*idx = i;
			return 0;
		}
	}

	return -ENOENT;
}

int wgm_iface_peer_add_unique(struct wgm_iface *iface, const struct wgm_peer *peer)
{
	size_t idx;
	int ret;

	ret = wgm_iface_peer_find(iface, peer->public_key, &idx);
	if (!ret)
		return 0;

	if (ret != -ENOENT)
		return ret;

	return wgm_iface_peer_add(iface, peer);
}
