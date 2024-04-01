// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_PEER_H
#define WGM__WG_PEER_H

#include "helpers.h"

#include <stdint.h>

struct wgm_peer {
	char		public_key[128];
	char		**allowed_ips;
	uint16_t	nr_allowed_ips;
};

#endif /* #ifndef WGM__WG_PEER_H */
