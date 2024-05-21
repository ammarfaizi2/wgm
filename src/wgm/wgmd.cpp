// SPDX-License-Identifier: GPL-2.0-only

#include "wgmd.hpp"
#include "helpers.hpp"

namespace wgm {

wgmd_ctx::wgmd_ctx(const std::string &config_file_path,
		   const std::string &client_cfg_dir):
	config_file_path_(config_file_path),
	client_cfg_dir_(client_cfg_dir)
{
}

void wgmd_ctx::load_config(void)
{
	file_t file(config_file_path_, false, LOCK_EX);
}

wgmd_ctx::~wgmd_ctx(void) = default;

} /* namespace wgm */

using wgmd_ctx = wgm::wgmd_ctx;

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	wgmd_ctx ctx("./wg/config.json", "./wg/clients");

	return 0;
}
