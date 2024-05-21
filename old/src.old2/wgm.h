// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_WGM_H
#define WGM__WG_WGM_H

#include "wgm_helpers.h"

#include <stdbool.h>
#include <stdint.h>

struct wgm_ctx {
	char	*data_dir;
	char	*wg_quick_path;
	char	*wg_conf_path;
};

#endif /* #ifndef WGM__WG_WGM_H */
