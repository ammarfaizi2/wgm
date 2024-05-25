// SPDX-License-Identifier: GPL-2.0-only

#include <iostream>

#include <wgm/wgmd_ctx.hpp>

namespace wgm {

wgmd_ctx::wgmd_ctx(const std::string &config_file_path,
		   const std::string &client_cfg_dir):
	config_file_path_(config_file_path),
	client_cfg_dir_(client_cfg_dir)
{
	load_config();
}

void wgmd_ctx::load_config(void)
{
	size_t i;

	config_file_ = std::make_unique<file_t>(config_file_path_);
	json servers = config_file_->get_json();

	for (i = 0; i < servers.size(); i++) {
		try {
			const json &s = servers.at(i);
			server_config_t cfg(s);

			servers_.emplace(cfg.location, cfg);
		} catch (const std::exception &e) {
			pr_warn("Failed to parse server config at index %zu: %s, skipping...\n",
				i, e.what());
		}
	}
}

wgmd_ctx::~wgmd_ctx(void) = default;

} /* namespace wgm */
