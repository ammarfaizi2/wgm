// SPDX-License-Identifier: GPL-2.0-only

#include <iostream>

#include <wgm/wgmd_ctx.hpp>

namespace wgm {

wgmd_ctx::wgmd_ctx(const std::string &config_file_path,
		   const std::string &client_cfg_dir):
	config_file_path_(config_file_path),
	client_cfg_dir_(client_cfg_dir),
	config_file_(std::make_unique<file_t>(config_file_path_))
{
	load_config();
}

void wgmd_ctx::load_config(void)
{
	json servers = config_file_->get_json();
	size_t i;

	for (i = 0; i < servers.size(); i++) {
		try {
			const json &s = servers.at(i);
			const std::string location = s.at("Location");

			if (servers_.find(location) != servers_.end()) {
				pr_warn("Duplicate server config at index %zu, skipping...\n", i);
				continue;
			}

			servers_.emplace(location, s);
		} catch (const std::exception &e) {
			pr_warn("Failed to parse server config at index %zu: %s, skipping...\n", i, e.what());
		}
	}
}

void wgmd_ctx::dump(void) const
{
	for (const auto &s : servers_) {
		std::cout << s.first << std::endl;
		s.second.dump();
	}
}

wgmd_ctx::~wgmd_ctx(void) = default;

} /* namespace wgm */
