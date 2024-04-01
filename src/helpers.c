
#include "helpers.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <json-c/json.h>
#include <linux/if.h>
#include <stdarg.h>

void wgm_log_err(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

int load_str_array_from_json(char ***array, uint16_t *nr, const json_object *jobj)
{
	json_object *tmp;
	const char *tstr;
	size_t i;

	*nr = json_object_array_length(jobj);
	if (!*nr)
		return 0;

	*array = malloc(*nr * sizeof(char *));
	if (!*array)
		return -ENOMEM;

	for (i = 0; i < *nr; i++) {
		tmp = json_object_array_get_idx(jobj, i);
		if (!tmp || !json_object_is_type(tmp, json_type_string)) {
			wgm_log_err("Error: invalid array element\n");
			goto err;
		}

		tstr = json_object_get_string(tmp);
		if (!tstr) {
			wgm_log_err("Error: invalid array element\n");
			goto err;
		}

		(*array)[i] = strdup(tstr);
		if (!(*array)[i]) {
			wgm_log_err("Error: failed to allocate memory\n");
			goto err;
		}
	}

	return 0;

err:
	while (i--)
		free((*array)[i]);

	free(*array);
	return -ENOMEM;
}

char *strncpyl(char *dest, const char *src, size_t n)
{
	strncpy(dest, src, n - 1);
	dest[n - 1] = '\0';
	return dest;
}

int mkdir_recursive(const char *path, mode_t mode)
{
	char *p, *tmp;
	int ret;

	tmp = strdup(path);
	if (!tmp)
		return -ENOMEM;

	p = tmp;
	while (*p) {
		if (*p != '/') {
			p++;
			continue;
		}

		*p = '\0';
		ret = mkdir(tmp, mode);
		if (ret < 0) {
			ret = -errno;
			if (ret != -EEXIST) {
				free(tmp);
				return ret;
			}
		}
		*p = '/';
		p++;
	}

	ret = mkdir(tmp, mode);
	free(tmp);
	if (ret < 0) {
		ret = -errno;
		if (ret != -EEXIST)
			return ret;
	}

	return 0;
}

void free_str_array(char **array, size_t nr)
{
	size_t i;

	if (!array)
		return;

	for (i = 0; i < nr; i++)
		free(array[i]);

	free(array);
}

json_object *json_object_new_from_str_array(char **array, uint16_t nr)
{
	json_object *jobj;
	size_t i;

	jobj = json_object_new_array();
	if (!jobj)
		return NULL;

	for (i = 0; i < nr; i++)
		json_object_array_add(jobj, json_object_new_string(array[i]));

	return jobj;
}

int wgm_parse_ifname(const char *ifname, char *buf)
{
	size_t i, len;

	if (strlen(ifname) >= IFNAMSIZ) {
		printf("Error: interface name is too long, max length is %d\n", IFNAMSIZ - 1);
		return -1;
	}

	strncpy(buf, ifname, IFNAMSIZ - 1);
	buf[IFNAMSIZ - 1] = '\0';

	/*
	 * Validate the interface name:
	 *   - Must be a valid interface name.
	 *   - Valid characters: [a-zA-Z0-9\-]
	 */
	len = strlen(buf);
	for (i = 0; i < len; i++) {
		if ((buf[i] >= 'a' && buf[i] <= 'z') ||
		    (buf[i] >= 'A' && buf[i] <= 'Z') ||
		    (buf[i] >= '0' && buf[i] <= '9') ||
		    buf[i] == '-') {
			continue;
		}

		printf("Error: invalid interface name: %s\n", buf);
		return -1;
	}

	return 0;
}

int wgm_parse_key(const char *key, char *buf, size_t size)
{
	if (strlen(key) >= size) {
		printf("Error: private key is too long, max length is %zu\n", size - 1);
		return -1;
	}

	strncpy(buf, key, size - 1);
	buf[size - 1] = '\0';
	return 0;
}

int wgm_str_array_to_json(const struct wgm_str_array *arr, json_object **jobj)
{
	json_object *ret;
	size_t i;

	ret = json_object_new_array();
	if (!ret)
		return -ENOMEM;

	for (i = 0; i < arr->nr; i++) {
		if (json_object_array_add(ret, json_object_new_string(arr->arr[i])) < 0) {
			wgm_log_err("Error: wgm_str_array_to_json: failed to add string to JSON array\n");
			json_object_put(ret);
			return -ENOMEM;
		}
	}

	*jobj = ret;
	return 0;
}

int wgm_json_to_str_array(struct wgm_str_array *arr, json_object *jobj)
{
	size_t i;

	if (!json_object_is_type(jobj, json_type_array)) {
		wgm_log_err("Error: wgm_json_to_str_array: JSON object is not an array\n");
		return -EINVAL;
	}

	arr->nr = json_object_array_length(jobj);
	if (!arr->nr)
		return 0;

	arr->arr = malloc(arr->nr * sizeof(char *));
	if (!arr->arr)
		return -ENOMEM;

	for (i = 0; i < arr->nr; i++) {
		json_object *tmp = json_object_array_get_idx(jobj, i);

		if (!json_object_is_type(tmp, json_type_string)) {
			wgm_log_err("Error: wgm_json_to_str_array: invalid JSON array element\n");
			goto err;
		}

		arr->arr[i] = strdup(json_object_get_string(tmp));
		if (!arr->arr[i]) {
			wgm_log_err("Error: wgm_json_to_str_array: failed to allocate memory\n");
			goto err;
		}
	}

	return 0;

err:
	while (i--)
		free(arr->arr[i]);

	free(arr->arr);
	return -ENOMEM;
}

void wgm_str_array_free(struct wgm_str_array *arr)
{
	size_t i;

	if (!arr || !arr->arr)
		return;

	for (i = 0; i < arr->nr; i++)
		free(arr->arr[i]);

	free(arr->arr);
	memset(arr, 0, sizeof(*arr));
}

void wgm_str_array_dump(const struct wgm_str_array *arr)
{
	size_t i;

	if (!arr || !arr->arr)
		return;

	for (i = 0; i < arr->nr; i++)
		printf("  %s\n", arr->arr[i]);
}
