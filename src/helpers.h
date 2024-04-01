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

int mkdir_recursive(const char *path, mode_t mode);
char *strncpyl(char *dest, const char *src, size_t n);
int load_str_array_from_json(char ***array, uint16_t *nr, const json_object *jobj);
void free_str_array(char **array, size_t nr);
json_object *json_object_new_from_str_array(char **array, uint16_t nr);

int wgm_parse_ifname(const char *ifname, char *buf);
int wgm_parse_key(const char *key, char *buf, size_t size);

#endif /* #ifndef WGM__HELPERS_H */
