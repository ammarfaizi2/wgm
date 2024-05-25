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
	config_file_ = std::make_unique<file_t>(config_file_path_);
	config_ = config_file_->get_json();

	for (const auto &server : config_["servers"])
		servers_.emplace(server["location"].get<std::string>(),
				 server_config_t(server));
}

wgmd_ctx::~wgmd_ctx(void) = default;

} /* namespace wgm */
