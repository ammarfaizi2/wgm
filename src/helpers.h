// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__HELPERS_H
#define WGM__HELPERS_H

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <arpa/inet.h>

struct wgm_str_array {
	char		**arr;
	size_t		nr;
};

int mkdir_recursive(const char *path, mode_t mode);
char *strncpyl(char *dest, const char *src, size_t n);

void wgm_log_err(const char *fmt, ...);

int wgm_parse_ifname(const char *ifname, char *buf);
int wgm_parse_key(const char *key, char *buf, size_t size);

int wgm_str_array_to_json(const struct wgm_str_array *arr, json_object **jobj);
int wgm_json_to_str_array(struct wgm_str_array *arr, json_object *jobj);
void wgm_str_array_free(struct wgm_str_array *arr);

void wgm_str_array_dump(const struct wgm_str_array *arr);

#endif /* #ifndef WGM__HELPERS_H */
