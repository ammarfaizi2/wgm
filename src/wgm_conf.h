// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_CONF_H
#define WGM__WG_CONF_H

#include "helpers.h"
#include "wgm.h"
#include "wgm_iface.h"
#include "wgm_peer.h"

int wgm_conf_save(const struct wgm_iface *iface, struct wgm_ctx *ctx);

#endif /* #ifndef WGM__WG_CONF_H */
