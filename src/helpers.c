// SPDX-License-Identifier: GPL-2.0-only

#include "helpers.h"

#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>

void wgm_log_err(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

char *strncpyl(char *dest, const char *src, size_t n)
{
	strncpy(dest, src, n - 1);
	dest[n - 1] = '\0';
	return dest;
}

static int __mkdir_exist_ok(const char *path, mode_t mode)
{
	int ret;

	ret = mkdir(path, mode);
	if (ret < 0) {
		ret = -errno;
		if (ret != -EEXIST)
			return ret;
	}

	return 0;
}

int mkdir_recursive(const char *path, mode_t mode)
{
	char *tmp, *p;
	int ret;

	tmp = strdup(path);
	if (!tmp)
		return -ENOMEM;

	for (p = tmp + 1; *p; p++) {
		if (*p != '/')
			continue;
		
		*p = '\0';
		ret = __mkdir_exist_ok(tmp, mode);
		if (ret < 0)
			goto out;
		*p = '/';
	}

	ret = __mkdir_exist_ok(tmp, mode);
out:
	free(tmp);
	return ret;
}

int wgm_create_getopt_long_args(struct option **long_opt_p, char **short_opt_p,
				const struct wgm_opt *opts, size_t nr_opts)
{
	struct option *lo;
	size_t i, j;
	char *so;

	lo = malloc(sizeof(*lo) * (nr_opts + 1));
	if (!lo)
		return -ENOMEM;

	so = malloc(nr_opts * 2 + 1);
	if (!so) {
		free(lo);
		return -ENOMEM;
	}

	j = 0;
	for (i = 0; i < nr_opts; i++) {
		if (!opts[i].name)
			break;

		char *name = strdup(opts[i].name);
		if (!name)
			goto out_free;

		lo[i].name = name;
		lo[i].has_arg = opts[i].has_arg;
		lo[i].flag = opts[i].flag;
		lo[i].val = opts[i].val;
		so[j++] = opts[i].val;
		if (opts[i].has_arg != no_argument)
			so[j++] = ':';
	}

	so[j] = '\0';
	memset(&lo[i], 0, sizeof(*lo));
	*long_opt_p = lo;
	*short_opt_p = so;
	return 0;

out_free:
	while (i--)
		free((char *)lo[i].name);

	free(so);
	free(lo);
	return -ENOMEM;
}

void wgm_free_getopt_long_args(struct option *long_opt, char *short_opt)
{
	size_t i;

	for (i = 0; long_opt[i].name; i++)
		free((char *)long_opt[i].name);

	free(short_opt);
	free(long_opt);
}

int wgm_parse_csv(struct wgm_str_array *arr, const char *str)
{
	char *start, *tmp;
	size_t i;

	tmp = strdup(str);
	if (!tmp)
		return -ENOMEM;

	memset(arr, 0, sizeof(*arr));
	start = tmp;
	for (i = 0;; i++) {
		char *end, c = tmp[i];

		if (c != ',' && c != '\0')
			continue;

		while (isspace(*start))
			start++;

		end = &tmp[i];
		*end = '\0';
		while (isspace(*(end - 1)))
			*(--end) = '\0';

		if (wgm_str_array_add(arr, start)) {
			wgm_str_array_free(arr);
			free(tmp);
			return -ENOMEM;
		}

		start = end + 1;
		if (!c)
			break;
	}

	free(tmp);
	return 0;
}

int wgm_str_array_from_json(struct wgm_str_array *arr, const json_object *jobj)
{
	size_t i, nr;

	if (!json_object_is_type(jobj, json_type_array))
		return -EINVAL;

	nr = json_object_array_length(jobj);
	for (i = 0; i < nr; i++) {
		json_object *jstr = json_object_array_get_idx(jobj, i);

		if (!json_object_is_type(jstr, json_type_string)) {
			wgm_str_array_free(arr);
			return -EINVAL;
		}

		if (wgm_str_array_add(arr, json_object_get_string(jstr))) {
			wgm_str_array_free(arr);
			return -ENOMEM;
		}
	}

	return 0;
}

int wgm_str_array_to_json(json_object **jobj, const struct wgm_str_array *arr)
{
	json_object *jarr;
	size_t i;

	jarr = json_object_new_array();
	if (!jarr)
		return -ENOMEM;

	for (i = 0; i < arr->nr; i++) {
		json_object *jstr;

		jstr = json_object_new_string(arr->arr[i]);
		if (!jstr) {
			json_object_put(jarr);
			return -ENOMEM;
		}

		json_object_array_add(jarr, jstr);
	}

	*jobj = jarr;
	return 0;
}

void wgm_str_array_free(struct wgm_str_array *arr)
{
	size_t i;

	for (i = 0; i < arr->nr; i++)
		free(arr->arr[i]);

	free(arr->arr);
	memset(arr, 0, sizeof(*arr));
}

int wgm_str_array_copy(struct wgm_str_array *dst, const struct wgm_str_array *src)
{
	char **arr;
	size_t i;

	arr = malloc(src->nr * sizeof(char *));
	if (!arr)
		return -ENOMEM;

	for (i = 0; i < src->nr; i++) {
		arr[i] = strdup(src->arr[i]);
		if (!arr[i]) {
			while (i--)
				free(arr[i]);
			free(arr);
			return -ENOMEM;
		}
	}

	dst->arr = arr;
	dst->nr = src->nr;
	return 0;
}

int wgm_str_array_add(struct wgm_str_array *arr, const char *str)
{
	char *dstr, **new_arr;
	size_t new_nr;

	dstr = strdup(str);
	if (!dstr)
		return -ENOMEM;

	new_nr = arr->nr + 1;
	new_arr = realloc(arr->arr, new_nr * sizeof(char *));
	if (!new_arr) {
		free(dstr);
		return -ENOMEM;
	}

	new_arr[new_nr - 1] = dstr;
	arr->arr = new_arr;
	arr->nr = new_nr;
	return 0;
}

int wgm_str_array_del(struct wgm_str_array *arr, size_t idx)
{
	if (idx >= arr->nr)
		return -EINVAL;

	free(arr->arr[idx]);
	memmove(&arr->arr[idx], &arr->arr[idx + 1], (arr->nr - idx - 1) * sizeof(char *));
	arr->nr--;

	if (!arr->nr) {
		free(arr->arr);
		memset(arr, 0, sizeof(*arr));
	} else {
		char **new_arr;

		new_arr = realloc(arr->arr, arr->nr * sizeof(char *));
		if (new_arr)
			arr->arr = new_arr;
	}

	return 0;
}

int wgm_str_array_move(struct wgm_str_array *dst, struct wgm_str_array *src)
{
	dst->arr = src->arr;
	dst->nr = src->nr;
	memset(src, 0, sizeof(*src));
	return 0;
}
