
#include "helpers.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <json-c/json.h>

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
