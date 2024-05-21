// SPDX-License-Identifier: GPL-2.0-only

#include <wgm/wgmd_ctx.hpp>

namespace wgm {

wgmd_ctx::wgmd_ctx(const std::string &config_file_path,
		   const std::string &client_cfg_dir):
	config_file_path_(config_file_path),
	client_cfg_dir_(client_cfg_dir)
{
}

void wgmd_ctx::load_config(void)
{
}

wgmd_ctx::~wgmd_ctx(void) = default;

} /* namespace wgm */
