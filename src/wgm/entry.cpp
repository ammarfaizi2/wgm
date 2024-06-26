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
#include <getopt.h>

#include <sys/file.h>

static struct option long_options[] = {
	{"config-file",		required_argument, 0, 'c'},
	{"client-cfg-dir",	required_argument, 0, 'd'},
	{"wg-conn-dir",		required_argument, 0, 'w'},
	{"wg-dir",		required_argument, 0, 'g'},
	{"ipt-path",		required_argument, 0, 'i'},
	{"ip2-path",		required_argument, 0, 'p'},
	{"true-path",		required_argument, 0, 't'},
	{"wg-quick-path",	required_argument, 0, 'q'},
	{"atomic-run-file",	required_argument, 0, 'a'},
	{0, 0, 0, 0}
};

int main(int argc, char *argv[])
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
	const char *atomic_run_file = "/tmp/wgm_run.lock";
#else
	const char *cfg_file = "/tmp/wg/config.json";
	const char *client_cfg_dir = "/tmp/wg/clients";
	const char *wg_conn_dir = "/tmp/wg_connections";
	const char *wg_dir = "/etc/wireguard";
	const char *ipt_path = "/usr/sbin/iptables";
	const char *ip2_path = "/usr/sbin/ip";
	const char *true_path = "/usr/bin/true";
	const char *wg_quick_path = "/usr/bin/wg-quick";
	const char *atomic_run_file = "/tmp/wgm_run.lock";
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

	env = getenv("WGMD_ATOMIC_RUN");
	if (env)
		atomic_run_file = env;

	int c;

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "c:d:w:g:i:p:t:q:a:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'c':
			cfg_file = optarg;
			break;
		case 'd':
			client_cfg_dir = optarg;
			break;
		case 'w':
			wg_conn_dir = optarg;
			break;
		case 'g':
			wg_dir = optarg;
			break;
		case 'i':
			ipt_path = optarg;
			break;
		case 'p':
			ip2_path = optarg;
			break;
		case 't':
			true_path = optarg;
			break;
		case 'q':
			wg_quick_path = optarg;
			break;
		case 'a':
			atomic_run_file = optarg;
			break;
		default:
			printf("Usage: %s [options]\n", "wgm");
			printf("Options:\n");
			printf("  --config-file, -c <file>     Set the config.json file (default: %s)\n", cfg_file);
			printf("  --client-cfg-dir, -d <dir>   Set the client config directory (default: %s)\n", client_cfg_dir);
			printf("  --wg-conn-dir, -w <dir>      Set the WireGuard connection directory (default: %s)\n", wg_conn_dir);
			printf("  --wg-dir, -g <dir>           Set the WireGuard configuration directory (default: %s)\n", wg_dir);
			printf("  --ipt-path, -i <path>        Set the iptables path (default: %s)\n", ipt_path);
			printf("  --ip2-path, -p <path>        Set the ip path (default: %s)\n", ip2_path);
			printf("  --true-path, -t <path>       Set the true path (default: %s)\n", true_path);
			printf("  --wg-quick-path, -q <path>   Set the wg-quick path (default: %s)\n", wg_quick_path);
			printf("  --atomic-run-file, -a <file> Set the atomic run file to avoid process duplication (default: %s)\n", atomic_run_file);
			printf("\n");
			/*
			 * Run example:
			 * ./wgmd --config-file /tmp/wg/config.json --client-cfg-dir /tmp/wg/clients --wg-conn-dir /tmp/wg_connections --wg-dir /etc/wireguard --ipt-path /usr/sbin/iptables --ip2-path /usr/sbin/ip --true-path /usr/bin/true --wg-quick-path /usr/bin/wg-quick
			 */
			return 1;
		}
	}


	FILE *lock_file = NULL;

	try {
		int ret;

		lock_file = fopen(atomic_run_file, "ab");
		if (!lock_file) {
			fprintf(stderr, "Failed to open atomic run file: '%s': %s\n", atomic_run_file, strerror(errno));
			return 1;
		}

		if (flock(fileno(lock_file), LOCK_EX | LOCK_NB) < 0) {
			fprintf(stderr, "Another wgmd is already running using a lock file: '%s', exiting...\n", atomic_run_file);
			fclose(lock_file);
			return 1;
		}

		wgm::ctx c(cfg_file, client_cfg_dir, wg_conn_dir, wg_dir,
			   ipt_path, ip2_path, true_path, wg_quick_path);

		ret = c.run();
		fclose(lock_file);
		return ret;
	} catch (const std::exception &e) {
		fprintf(stderr, "Error: %s\n", e.what());
		if (lock_file)
			fclose(lock_file);

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

std::vector<std::string> str_explode(const std::string &str, const std::string &delim,
				     unsigned int limit)
{
	std::vector<std::string> ret;
	size_t pos = 0;
	size_t start = 0;
	size_t len = str.size();
	size_t delim_len = delim.size();
	unsigned int count = 0;

	while (pos < len) {
		pos = str.find(delim, start);
		if (pos == std::string::npos)
			pos = len;

		ret.push_back(str.substr(start, pos - start));
		start = pos + delim_len;

		if (limit && ++count >= limit)
			break;
	}

	return ret;
}

} /* namespace wgm */
