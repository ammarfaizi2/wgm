// SPDX-License-Identifier: GPL-2.0-only

#include <string.h>

#include "wgm_cmd_peer.h"

void wgm_cmd_peer_show_usage(const char *app, int show_cmds)
{
}

int wgm_cmd_peer(int argc, char *argv[], struct wgm_ctx *ctx)
{
	return 0;
}

int wgm_peer_to_json(const struct wgm_peer *peer, json_object **ret)
{
	json_object *tmp;
	int err = 0;

	*ret = json_object_new_object();
	if (!*ret)
		return -ENOMEM;

	err |= json_object_object_add(*ret, "public_key", json_object_new_string(peer->public_key));
	if (err)
		goto out_err;

	err = wgm_array_str_to_json(&peer->addresses, &tmp);
	if (err)
		goto out_err;

	err = json_object_object_add(*ret, "addresses", tmp);
	if (err)
		goto out_err;

	return 0;

out_err:
	json_object_put(*ret);
	return -ENOMEM;
}

int wgm_peer_from_json(struct wgm_peer *peer, const json_object *obj)
{
	return 0;
}

int wgm_peer_copy(struct wgm_peer *dst, const struct wgm_peer *src)
{
	struct wgm_peer tmp;

	tmp = *src;

	if (wgm_array_str_copy(&tmp.ips, &src->ips))
		return -ENOMEM;

	*dst = tmp;
	return 0;
}

void wgm_peer_move(struct wgm_peer *dst, struct wgm_peer *src)
{
	*dst = *src;
	wgm_array_str_move(&dst->ips, &src->ips);
	wgm_peer_free(src);
}

void wgm_peer_free(struct wgm_peer *peer)
{
	wgm_array_str_free(&peer->ips);
	memset(peer, 0, sizeof(*peer));
}

int wgm_array_peer_add(struct wgm_array_peer *arr, const struct wgm_peer *peer)
{
	struct wgm_peer *new_arr;
	size_t new_len;

	new_len = arr->len + 1;
	new_arr = realloc(arr->arr, new_len * sizeof(*new_arr));
	if (!new_arr)
		return -ENOMEM;

	arr->arr = new_arr;
	if (wgm_peer_copy(&arr->arr[arr->len], peer))
		return -ENOMEM;

	arr->len = new_len;
	return 0;
}

int wgm_array_peer_del(struct wgm_array_peer *arr, size_t idx)
{
	if (idx >= arr->len)
		return -EINVAL;

	memmove(&arr->arr[idx], &arr->arr[idx + 1], (arr->len - idx - 1) * sizeof(*arr->arr));
	arr->len--;

	return 0;
}

int wgm_array_peer_find(struct wgm_array_peer *arr, const char *pub_key, size_t *idx)
{
	size_t i;

	for (i = 0; i < arr->len; i++) {
		if (strcmp(arr->arr[i].public_key, pub_key) == 0) {
			*idx = i;
			return 0;
		}
	}

	return -ENOENT;
}

int wgm_array_peer_add_unique(struct wgm_array_peer *arr, const struct wgm_peer *peer)
{
	size_t idx;
	int ret;

	ret = wgm_array_peer_find(arr, peer->public_key, &idx);
	if (ret == 0)
		return -EEXIST;

	return wgm_array_peer_add(arr, peer);
}

int wgm_array_peer_add_mv(struct wgm_array_peer *arr, struct wgm_peer *peer)
{
	int ret;

	ret = wgm_array_peer_add(arr, peer);
	if (ret)
		wgm_peer_free(peer);

	return ret;
}

int wgm_array_peer_add_mv_unique(struct wgm_array_peer *arr, struct wgm_peer *peer)
{
	int ret;

	ret = wgm_array_peer_add_unique(arr, peer);
	if (ret)
		wgm_peer_free(peer);

	return ret;
}

int wgm_array_peer_copy(struct wgm_array_peer *dst, const struct wgm_array_peer *src)
{
	struct wgm_peer *new_arr;
	size_t i;

	new_arr = malloc(src->len * sizeof(*new_arr));
	if (!new_arr)
		return -ENOMEM;

	for (i = 0; i < src->len; i++) {
		if (wgm_peer_copy(&new_arr[i], &src->arr[i])) {
			while (i--)
				wgm_peer_free(&new_arr[i]);
			free(new_arr);
			return -ENOMEM;
		}
	}

	dst->arr = new_arr;
	dst->len = src->len;
	return 0;
}

void wgm_array_peer_move(struct wgm_array_peer *dst, struct wgm_array_peer *src)
{
	*dst = *src;
	memset(src, 0, sizeof(*src));
}

void wgm_array_peer_free(struct wgm_array_peer *peer_array)
{
	size_t i;

	for (i = 0; i < peer_array->len; i++)
		wgm_peer_free(&peer_array->arr[i]);

	free(peer_array->arr);
	memset(peer_array, 0, sizeof(*peer_array));
}

int wgm_array_peer_to_json(const struct wgm_array_peer *arr, json_object **obj)
{
	json_object *tmp;
	size_t i;
	int err = 0;

	*obj = json_object_new_array();
	if (!*obj)
		return -ENOMEM;

	for (i = 0; i < arr->len; i++) {
		err = wgm_peer_to_json(&arr->arr[i], &tmp);
		if (err)
			goto out_err;

		err = json_object_array_add(*obj, tmp);
		if (err)
			goto out_err;
	}

	return 0;

out_err:
	json_object_put(*obj);
	return -ENOMEM;
}

int wgm_array_peer_from_json(struct wgm_array_peer *arr, const json_object *obj)
{
	return 0;
}
