// SPDX-License-Identifier: GPL-2.0-only

#include <wgm/wgmd_ctx.hpp>

using wgmd_ctx = wgm::wgmd_ctx;

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	wgmd_ctx ctx("./wg/config.json", "./wg/clients");

	return 0;
}
