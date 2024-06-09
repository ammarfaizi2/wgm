// SPDX-License-Identifier: GPL-2.0-only

#include <cstdio>
#include <exception>

#include <wgm/wgmd_ctx.hpp>

using wgmd_ctx = wgm::wgmd_ctx;

static void run_wgmd(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	wgmd_ctx ctx("./wg/config.json", "./wg/clients");
	ctx.dump();
}

int main(int argc, char *argv[])
{
	try {
		run_wgmd(argc, argv);
	} catch (const std::exception &e) {
		fprintf(stderr, "Error: %s\n", e.what());
		return 1;
	}

	return 0;
}
