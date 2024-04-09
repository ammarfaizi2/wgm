// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_HELPERS_H
#define WGM__WG_HELPERS_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <json-c/json.h>

#include <getopt.h>
#include <linux/if.h>
#include <sys/types.h>
#include <arpa/inet.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

struct wgm_str_array {
	char	**arr;
	size_t	nr;
};

struct wgm_ctx;

struct wgm_opt {
	uint64_t id;
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

void wgm_log_err(const char *fmt, ...);
char *strncpyl(char *dest, const char *src, size_t n);
int wgm_parse_csv(struct wgm_str_array *arr, const char *str);
int mkdir_recursive(const char *path, mode_t mode);
int wgm_create_getopt_long_args(struct option **long_opt_p, char **short_opt_p,
				const struct wgm_opt *opts, size_t nr_opts);
void wgm_free_getopt_long_args(struct option *long_opt, char *short_opt);

int wgm_str_array_from_json(struct wgm_str_array *arr, const json_object *jobj);
int wgm_str_array_to_json(json_object **jobj, const struct wgm_str_array *arr);
void wgm_str_array_free(struct wgm_str_array *arr);
int wgm_str_array_copy(struct wgm_str_array *dst, const struct wgm_str_array *src);
int wgm_str_array_add(struct wgm_str_array *arr, const char *str);
int wgm_str_array_del(struct wgm_str_array *arr, size_t idx);
int wgm_str_array_move(struct wgm_str_array *dst, struct wgm_str_array *src);
int wgm_asprintf(char **strp, const char *fmt, ...);
int wgm_get_realpath(const char *path, char **rp);
ssize_t wgm_copy_file(const char *src, const char *dst);
bool wgm_file_exists(const char *path);
bool wgm_cmp_file_md5(const char *f1, const char *f2);

#endif /* #ifndef WGM__WG_HELPERS_H */
