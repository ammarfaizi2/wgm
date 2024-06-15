// SPDX-License-Identifier: GPL-2.0-only

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <wgm/entry.hpp>
#include <wgm/ctx.hpp>

#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <stdexcept>

#include <dirent.h>

int main(void)
{
#if 0
	// Local testing only.
	const char *cfg_file = "./wg/config.json";
	const char *client_cfg_dir = "./wg/clients";
	const char *wg_conn_dir = "./wg_connections";
	const char *wg_dir = "/etc/wireguard";
	const char *ipt_path = "/usr/sbin/iptables";
	const char *ip2_path = "/usr/sbin/ip";
	const char *true_path = "/usr/bin/true";
	const char *wg_quick_path = "/usr/bin/wg-quick";
#else
	const char *cfg_file = "/tmp/wg/config.json";
	const char *client_cfg_dir = "/tmp/wg/clients";
	const char *wg_conn_dir = "/tmp/wg_connections";
	const char *wg_dir = "/etc/wireguard";
	const char *ipt_path = "/usr/sbin/iptables";
	const char *ip2_path = "/usr/sbin/ip";
	const char *true_path = "/usr/bin/true";
	const char *wg_quick_path = "/usr/bin/wg-quick";
#endif
	const char *env;

	env = getenv("WGMD_CONFIG_FILE");
	if (env)
		cfg_file = env;

	env = getenv("WGMD_CLIENT_CFG_DIR");
	if (env)
		client_cfg_dir = env;

	env = getenv("WGMD_WG_CONN_DIR");
	if (env)
		wg_conn_dir = env;

	env = getenv("WGMD_WG_DIR");
	if (env)
		wg_dir = env;

	env = getenv("WGMD_IPT_PATH");
	if (env)
		ipt_path = env;

	env = getenv("WGMD_IP2_PATH");
	if (env)
		ip2_path = env;

	env = getenv("WGMD_TRUE_PATH");
	if (env)
		true_path = env;

	env = getenv("WGMD_WG_QUICK_PATH");
	if (env)
		wg_quick_path = env;

	try {
		wgm::ctx c(cfg_file, client_cfg_dir, wg_conn_dir, wg_dir,
			   ipt_path, ip2_path, true_path, wg_quick_path);

		return c.run();
	} catch (const std::exception &e) {
		fprintf(stderr, "Error: %s\n", e.what());
		return 1;
	}
}

namespace wgm {

std::string load_str_from_file(const char *file)
{
	char tmp[4096];
	size_t fsize;
	FILE *f;

	f = fopen(file, "rb");
	if (!f) {
		snprintf(tmp, sizeof(tmp), "Failed to open file: '%s': %s\n", file, strerror(errno));
		throw std::runtime_error(std::string(tmp));
	}

	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	try {
		std::string ret(fsize, '\0');

		if (fread(&ret[0], 1, fsize, f) != fsize) {
			fclose(f);
			snprintf(tmp, sizeof(tmp), "Failed to read file: '%s': %s\n", file, strerror(errno));
			throw std::runtime_error(std::string(tmp));
		}

		fclose(f);
		return ret;
	} catch (const std::bad_alloc &e) {
		fclose(f);
		snprintf(tmp, sizeof(tmp), "Failed to allocate memory for file: '%s': %s\n", file, e.what());
		throw std::runtime_error(std::string(tmp));
	}
}

nlohmann::json load_json_from_file(const char *file)
{
	std::string str = load_str_from_file(file);
	return nlohmann::json::parse(str);
}

void store_str_to_file(const char *file, const std::string &str)
{
	char tmp[4096];
	FILE *f;

	f = fopen(file, "wb");
	if (!f) {
		snprintf(tmp, sizeof(tmp), "Failed to open file: '%s': %s\n", file, strerror(errno));
		throw std::runtime_error(std::string(tmp));
	}

	if (fwrite(str.c_str(), 1, str.size(), f) != str.size()) {
		fclose(f);
		snprintf(tmp, sizeof(tmp), "Failed to write to file: '%s': %s\n", file, strerror(errno));
		throw std::runtime_error(std::string(tmp));
	}

	fclose(f);
}

std::vector<std::string> scandir(const char *dir, bool skip_dot_files)
{
	std::vector<std::string> ret;
	DIR *d;
	struct dirent *de;

	d = opendir(dir);
	if (!d) {
		char tmp[4096];
		snprintf(tmp, sizeof(tmp), "Failed to open directory: '%s': %s\n", dir, strerror(errno));
		throw std::runtime_error(std::string(tmp));
	}

	try {
		while (1) {
			de = readdir(d);
			if (!de)
				break;

			if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
				continue;

			if (skip_dot_files && de->d_name[0] == '.')
				continue;

			ret.push_back(de->d_name);
		}

		closedir(d);
		return ret;
	} catch (const std::bad_alloc &e) {
		closedir(d);
		char tmp[4096];
		snprintf(tmp, sizeof(tmp), "Failed to allocate memory for directory: '%s': %s\n", dir, e.what());
		throw std::runtime_error(std::string(tmp));
	}
}

void pr_warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void pr_debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

} /* namespace wgm */
