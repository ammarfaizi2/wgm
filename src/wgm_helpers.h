// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WG_HELPERS_H
#define WGM__WG_HELPERS_H

#include <stdio.h>
#include <json-c/json.h>
#include <sys/file.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>

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

typedef struct wgm_global_lock {
	wgm_file_t	file;
} wgm_global_lock_t;

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

int wgm_asprintf(char **strp, const char *fmt, ...);
int wgm_vasprintf(char **strp, const char *fmt, va_list ap);
int wgm_mkdir_recursive(const char *path, mode_t mode);
int wgm_err_log(const char *fmt, ...);
int wgm_err_elog_add(const char *fmt, ...);
void wgm_err_elog_flush(void);

struct wgm_ctx;

int wgm_global_lock_open(wgm_global_lock_t *lock, struct wgm_ctx *ctx, const char *path);
int wgm_global_lock_close(wgm_global_lock_t *lock);

char *strncpyl(char *dest, const char *src, size_t n);

static inline int wgm_json_obj_kcp_str(const json_object *obj, const char *key,
				       char *buf, size_t len)
{
	json_object *val;
	const char *str;

	if (!json_object_object_get_ex(obj, key, &val))
		return -EINVAL;

	if (!json_object_is_type(val, json_type_string))
		return -EINVAL;

	str = json_object_get_string(val);
	strncpyl(buf, str, len);
	return 0;
}

static inline int wgm_json_obj_kcp_str_array(const json_object *obj, const char *key,
					     struct wgm_array_str *arr)
{
	json_object *val;

	if (!json_object_object_get_ex(obj, key, &val))
		return -EINVAL;

	if (!json_object_is_type(val, json_type_array))
		return -EINVAL;

	return wgm_array_str_from_json(arr, val);
}

#define DEF_WGM_JSON_INT_GETTER(type)					\
static inline int wgm_json_obj_kcp_##type(const json_object *obj,	\
						const char *key,	\
						type *val)		\
{									\
	json_object *jval;						\
									\
	if (!json_object_object_get_ex(obj, key, &jval))		\
		return -EINVAL;						\
									\
	if (!json_object_is_type(jval, json_type_int))			\
		return -EINVAL;						\
									\
	*val = (type)json_object_get_int64(jval);			\
	return 0;							\
}

DEF_WGM_JSON_INT_GETTER(int)
DEF_WGM_JSON_INT_GETTER(uint64_t)
DEF_WGM_JSON_INT_GETTER(uint32_t)
DEF_WGM_JSON_INT_GETTER(uint16_t)
DEF_WGM_JSON_INT_GETTER(uint8_t)

DEF_WGM_JSON_INT_GETTER(int64_t)
DEF_WGM_JSON_INT_GETTER(int32_t)
DEF_WGM_JSON_INT_GETTER(int16_t)
DEF_WGM_JSON_INT_GETTER(int8_t)

static inline int wgm_json_obj_set_str(json_object *obj, const char *key, const char *val)
{
	json_object *jval;
	int ret;

	jval = json_object_new_string(val);
	if (!jval)
		return -ENOMEM;

	ret = json_object_object_add(obj, key, jval);
	if (ret)
		json_object_put(jval);

	return ret;
}

/*
 * Only if the string is not empty.
 */
static inline int wgm_json_obj_set_str_ine(json_object *obj, const char *key, const char *val)
{
	if (!val || !val[0])
		return 0;

	return wgm_json_obj_set_str(obj, key, val);
}

static inline int wgm_json_obj_set_str_array(json_object *obj, const char *key,
					     const struct wgm_array_str *arr)
{
	json_object *val;
	int ret;

	ret = wgm_array_str_to_json(arr, &val);
	if (ret)
		return ret;

	ret = json_object_object_add(obj, key, val);
	if (ret)
		json_object_put(val);

	return ret;
}

#define DEF_WGM_JSON_INT_SETTER(type)					\
static inline int wgm_json_obj_set_##type(json_object *obj,		\
					  const char *key, type val)	\
{									\
	json_object *jval;						\
	int ret;							\
									\
	jval = json_object_new_int64(val);				\
	if (!jval)							\
		return -ENOMEM;						\
									\
	ret = json_object_object_add(obj, key, jval);			\
	if (ret)							\
		json_object_put(jval);					\
									\
	return ret;							\
}

DEF_WGM_JSON_INT_SETTER(int)
DEF_WGM_JSON_INT_SETTER(uint64_t)
DEF_WGM_JSON_INT_SETTER(uint32_t)
DEF_WGM_JSON_INT_SETTER(uint16_t)
DEF_WGM_JSON_INT_SETTER(uint8_t)

DEF_WGM_JSON_INT_SETTER(int64_t)
DEF_WGM_JSON_INT_SETTER(int32_t)
DEF_WGM_JSON_INT_SETTER(int16_t)
DEF_WGM_JSON_INT_SETTER(int8_t)

#endif /* #ifndef WGM__WG_HELPERS_H */
