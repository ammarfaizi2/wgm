
#include "helpers.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <json-c/json.h>
#include <linux/if.h>

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
			fprintf(stderr, "Error: invalid array element\n");
			goto err;
		}

		tstr = json_object_get_string(tmp);
		if (!tstr) {
			fprintf(stderr, "Error: invalid array element\n");
			goto err;
		}

		(*array)[i] = strdup(tstr);
		if (!(*array)[i]) {
			fprintf(stderr, "Error: failed to allocate memory\n");
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
