// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WGMD_HPP
#define WGM__WGMD_HPP

#include <memory>
#include <string>
#include <cstdio>
#include <unordered_map>

#include <wgm/helpers.hpp>
#include <wgm/server_config.hpp>

namespace wgm {

class wgmd_ctx {
private:

	using json = nlohmann::json;

	std::string config_file_path_;
	std::string client_cfg_dir_;
	std::unique_ptr<file_t> config_file_;

	std::unordered_map<std::string, server_config_t> servers_;

	inline void load_config(void);
	inline void load_clients(void);

public:
	wgmd_ctx(const std::string &config_file_path,
		 const std::string &client_cfg_dir);

	~wgmd_ctx(void);
};

#ifndef pr_warn
#define pr_warn(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif

} /* namespace wgm */

#endif /* #ifndef WGM__WGMD_HPP */
