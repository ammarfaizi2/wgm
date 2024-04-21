// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_HELPERS_H
#define WGM__WG_HELPERS_H

#include <stdio.h>
#include <json-c/json.h>
#include <errno.h>
#include <getopt.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

struct wgm_array_str {
	char	**arr;
	size_t	len;
};

struct wgm_opt {
	uint64_t id;
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

typedef struct wgm_file {
	FILE	*file;
} wgm_file_t;

#define WGM_JSON_FLAGS (JSON_C_TO_STRING_NOSLASHESCAPE | JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY)

int wgm_array_str_add(struct wgm_array_str *arr, const char *str);
int wgm_array_str_del(struct wgm_array_str *arr, size_t idx);
int wgm_array_str_find(struct wgm_array_str *arr, const char *str, size_t *idx);
int wgm_array_str_add_unique(struct wgm_array_str *arr, const char *str);
int wgm_array_str_copy(struct wgm_array_str *dst, const struct wgm_array_str *src);
void wgm_array_str_move(struct wgm_array_str *dst, struct wgm_array_str *src);
void wgm_array_str_free(struct wgm_array_str *str_array);

int wgm_array_str_to_json(const struct wgm_array_str *arr, json_object **obj);
int wgm_array_str_from_json(struct wgm_array_str *arr, const json_object *obj);

int wgm_array_str_to_csv(const struct wgm_array_str *arr, char **csv);
int wgm_array_str_from_csv(struct wgm_array_str *arr, const char *csv);

int wgm_file_open(wgm_file_t *file, const char *path, const char *mode);
int wgm_file_open_lock(wgm_file_t *file, const char *path, const char *mode, int type);
int wgm_file_lock(wgm_file_t *file, int type);
int wgm_file_unlock(wgm_file_t *file);
int wgm_file_close(wgm_file_t *file);

int wgm_file_get_contents(wgm_file_t *file, char **contents, size_t *len);
int wgm_file_put_contents(wgm_file_t *file, const char *contents, size_t len);

int wgm_create_getopt_long_args(struct option **long_opt_p, char **short_opt_p,
				const struct wgm_opt *opts, size_t nr_opts);
void wgm_free_getopt_long_args(struct option *long_opt, char *short_opt);

#endif /* #ifndef WGM__WG_HELPERS_H */
