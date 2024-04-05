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

int wgm_parse_csv(struct wgm_str_array *arr, const char *str_ips)
{
	char *tmp, *p, **str_arr;
	size_t i, n, len;
	int ret;

	memset(arr, 0, sizeof(*arr));
	if (!str_ips)
		return 0;

	tmp = strdup(str_ips);
	if (!tmp)
		return -ENOMEM;

	len = strlen(tmp);
	n = 0;
	for (i = 0; i < len; i++) {
		if (tmp[i] == ',') {
			n++;
			tmp[i] = '\0';
		}
	}

	n++;

	str_arr = calloc(n, sizeof(char *));
	if (!str_arr) {
		ret = -ENOMEM;
		goto out;
	}

	p = tmp;

	for (i = 0; i < n; i++) {
		str_arr[i] = strdup(p);
		if (!str_arr[i]) {
			ret = -ENOMEM;
			goto out;
		}
		p += strlen(p) + 1;
	}

	arr->arr = str_arr;
	arr->nr = n;
	ret = 0;

out:
	if (ret < 0) {
		while (n--)
			free(str_arr[n]);
		free(str_arr);
	}

	free(tmp);
	return ret;
}

int wgm_json_to_str_array(struct wgm_str_array *arr, json_object *jobj)
{
	json_object *jstr;
	size_t i, n;
	int ret;

	memset(arr, 0, sizeof(*arr));
	if (!jobj)
		return 0;

	if (!json_object_is_type(jobj, json_type_array))
		return -EINVAL;

	n = json_object_array_length(jobj);
	if (!n)
		return 0;

	arr->arr = calloc(n, sizeof(char *));
	if (!arr->arr)
		return -ENOMEM;

	for (i = 0; i < n; i++) {
		jstr = json_object_array_get_idx(jobj, i);
		if (!jstr) {
			ret = -ENOMEM;
			goto out_err;
		}

		if (!json_object_is_type(jstr, json_type_string)) {
			ret = -EINVAL;
			goto out_err;
		}

		arr->arr[i] = strdup(json_object_get_string(jstr));
		if (!arr->arr[i]) {
			ret = -ENOMEM;
			goto out_err;
		}
	}

	arr->nr = n;
	return 0;

out_err:
	while (i--)
		free(arr->arr[i]);
	free(arr->arr);
	memset(arr, 0, sizeof(*arr));
	return ret;
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

void wgm_free_str_array(struct wgm_str_array *arr)
{
	size_t i;

	for (i = 0; i < arr->nr; i++)
		free(arr->arr[i]);

	free(arr->arr);
	memset(arr, 0, sizeof(*arr));
}
