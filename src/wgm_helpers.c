// SPDX-License-Identifier: GPL-2.0-only

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/file.h>

#include "wgm_helpers.h"

int wgm_array_str_add(struct wgm_array_str *arr, const char *str)
{
	size_t new_len;
	char **new_arr;

	if (!arr || !str)
		return -EINVAL;

	new_len = arr->len + 1;
	new_arr = realloc(arr->arr, new_len * sizeof(*arr->arr));
	if (!new_arr)
		return -ENOMEM;

	arr->arr = new_arr;
	arr->arr[arr->len] = strdup(str);
	if (!arr->arr[arr->len])
		return -ENOMEM;

	arr->len = new_len;
	return 0;
}

int wgm_array_str_del(struct wgm_array_str *arr, size_t idx)
{
	if (!arr || idx >= arr->len)
		return -EINVAL;

	free(arr->arr[idx]);
	memmove(&arr->arr[idx], &arr->arr[idx + 1], (arr->len - idx - 1) * sizeof(*arr->arr));
	arr->len--;
	return 0;

}

int wgm_array_str_find(struct wgm_array_str *arr, const char *str, size_t *idx)
{
	size_t i;

	if (!arr || !str || !idx)
		return -EINVAL;

	for (i = 0; i < arr->len; i++) {
		if (!strcmp(arr->arr[i], str)) {
			*idx = i;
			return 0;
		}
	}

	return -ENOENT;
}

int wgm_array_str_add_unique(struct wgm_array_str *arr, const char *str)
{
	size_t idx;

	if (!arr || !str)
		return -EINVAL;

	if (!wgm_array_str_find(arr, str, &idx))
		return -EEXIST;

	return wgm_array_str_add(arr, str);
}

int wgm_array_str_copy(struct wgm_array_str *dst, const struct wgm_array_str *src)
{
	size_t new_len, i;
	char **new_arr;

	if (!dst || !src)
		return -EINVAL;

	new_len = src->len;
	new_arr = malloc(new_len * sizeof(*new_arr));
	if (!new_arr)
		return -ENOMEM;

	for (i = 0; i < new_len; i++) {
		new_arr[i] = strdup(src->arr[i]);
		if (!new_arr[i]) {
			while (i--)
				free(new_arr[i]);
			free(new_arr);
			return -ENOMEM;
		}
	}

	dst->arr = new_arr;
	dst->len = new_len;

	return 0;
}

void wgm_array_str_free(struct wgm_array_str *arr)
{
	size_t i;

	if (!arr)
		return;

	for (i = 0; i < arr->len; i++)
		free(arr->arr[i]);

	free(arr->arr);
	memset(arr, 0, sizeof(*arr));
}

void wgm_array_str_move(struct wgm_array_str *dst, struct wgm_array_str *src)
{
	if (!dst || !src)
		return;

	wgm_array_str_free(dst);
	memcpy(dst, src, sizeof(*dst));
	memset(src, 0, sizeof(*src));
}

int wgm_array_str_to_json(const struct wgm_array_str *arr, json_object **obj)
{
	json_object *arr_obj;
	size_t i;

	if (!arr || !obj)
		return -EINVAL;

	arr_obj = json_object_new_array();
	if (!arr_obj)
		return -ENOMEM;

	for (i = 0; i < arr->len; i++) {
		json_object *str_obj = json_object_new_string(arr->arr[i]);
		if (!str_obj) {
			json_object_put(arr_obj);
			return -ENOMEM;
		}

		json_object_array_add(arr_obj, str_obj);
	}

	*obj = arr_obj;
	return 0;
}

int wgm_array_str_from_json(struct wgm_array_str *arr, const json_object *obj)
{
	struct wgm_array_str tmp;
	size_t i, len, ret;

	if (!arr || !obj)
		return -EINVAL;

	memset(&tmp, 0, sizeof(tmp));
	len = json_object_array_length(obj);

	for (i = 0; i < len; i++) {
		json_object *str_obj = json_object_array_get_idx(obj, i);
		const char *str;

		ret = -EINVAL;
		if (!str_obj)
			goto out_err;
		if (json_object_get_type(str_obj) != json_type_string)
			goto out_err;
		str = json_object_get_string(str_obj);
		if (!str)
			goto out_err;
		ret = wgm_array_str_add(&tmp, str);
		if (ret)
			goto out_err;
	}

	wgm_array_str_move(arr, &tmp);
	return 0;

out_err:
	wgm_array_str_free(&tmp);
	return 0;
}

int wgm_array_str_to_csv(const struct wgm_array_str *arr, char **csv)
{
	size_t i, len;
	char *buf, *ptr;

	if (!arr || !csv)
		return -EINVAL;

	len = 0;
	for (i = 0; i < arr->len; i++)
		len += strlen(arr->arr[i]) + 1;

	buf = malloc(len);
	if (!buf)
		return -ENOMEM;

	ptr = buf;
	for (i = 0; i < arr->len; i++) {
		strcpy(ptr, arr->arr[i]);
		ptr += strlen(arr->arr[i]);
		*ptr++ = ',';
	}

	*--ptr = '\0';
	*csv = buf;
	return 0;
}

static char *trim_str(char *str)
{
	char *end;

	while (*str == ' ' || *str == '\t')
		str++;

	end = str + strlen(str) - 1;
	while (end > str && (*end == ' ' || *end == '\t'))
		end--;

	end[1] = '\0';
	return str;
}

int wgm_array_str_from_csv(struct wgm_array_str *arr, const char *csv)
{
	char *str, *str_copy, *saveptr;
	struct wgm_array_str tmp;
	size_t ret;

	if (!arr || !csv)
		return -EINVAL;

	memset(&tmp, 0, sizeof(tmp));
	str_copy = strdup(csv);
	if (!str_copy)
		return -ENOMEM;

	for (str = strtok_r(str_copy, ",", &saveptr); str; str = strtok_r(NULL, ",", &saveptr)) {
		ret = wgm_array_str_add(&tmp, trim_str(str));
		if (ret) {
			free(str_copy);
			wgm_array_str_free(&tmp);
			return ret;
		}
	}

	free(str_copy);
	wgm_array_str_move(arr, &tmp);
	return 0;

}

int wgm_file_open(wgm_file_t *file, const char *path, const char *mode)
{
	if (!file || !path || !mode)
		return -EINVAL;

	file->file = fopen(path, mode);
	if (!file->file)
		return -errno;

	return 0;
}

int wgm_file_lock(wgm_file_t *file, int type)
{
	if (!file || !file->file)
		return -EINVAL;

	if (flock(fileno(file->file), type))
		return -errno;

	return 0;
}

int wgm_file_open_lock(wgm_file_t *file, const char *path, const char *mode, int type)
{
	int ret;

	ret = wgm_file_open(file, path, mode);
	if (ret)
		return ret;

	ret = wgm_file_lock(file, type);
	if (ret) {
		wgm_file_close(file);
		return ret;
	}

	return 0;
}

int wgm_file_unlock(wgm_file_t *file)
{
	if (!file || !file->file)
		return -EINVAL;

	if (flock(fileno(file->file), LOCK_UN))
		return -errno;

	return 0;
}

int wgm_file_close(wgm_file_t *file)
{
	if (!file || !file->file)
		return -EINVAL;

	if (fclose(file->file))
		return -errno;

	file->file = NULL;
	return 0;
}

int wgm_file_get_contents(wgm_file_t *file, char **contents, size_t *len)
{
	size_t cur_pos;
	char *buf;
	long ret;

	if (!file || !file->file || !contents || !len)
		return -EINVAL;

	ret = ftell(file->file);
	if (ret < 0)
		return -errno;

	cur_pos = (size_t)ret;
	if (fseek(file->file, 0, SEEK_END))
		return -errno;

	ret = ftell(file->file);
	if (ret < 0) {
		ret = -errno;
		goto out_revert;
	}

	if (fseek(file->file, cur_pos, SEEK_SET)) {
		ret = -errno;
		goto out_revert;
	}

	buf = malloc(*len + 1);
	if (!buf) {
		ret = -ENOMEM;
		goto out_revert;
	}

	if (fread(buf, 1, *len, file->file) != *len) {
		ret = -EIO;
		goto out_free;
	}

	buf[*len] = '\0';
	*contents = buf;
	return 0;

out_free:
	free(buf);
out_revert:
	fseek(file->file, cur_pos, SEEK_SET);
	return ret;
}

int wgm_file_put_contents(wgm_file_t *file, const char *contents, size_t len)
{
	if (!file || !file->file || !contents)
		return -EINVAL;

	fseek(file->file, 0, SEEK_SET);
	if (fwrite(contents, 1, len, file->file) != len)
		return -EIO;

	return 0;
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
