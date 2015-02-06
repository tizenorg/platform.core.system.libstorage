/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "list.h"
#include "log.h"

const char *dir_path[STORAGE_DIRECTORY_MAX] = {
	[STORAGE_DIRECTORY_IMAGES] = "Images",
	[STORAGE_DIRECTORY_SOUNDS] = "Sounds",
	[STORAGE_DIRECTORY_VIDEOS] = "Videos",
	[STORAGE_DIRECTORY_CAMERA] = "DCIM",
	[STORAGE_DIRECTORY_DOWNLOADS] = "Downloads",
	[STORAGE_DIRECTORY_MUSIC] = "Music",
	[STORAGE_DIRECTORY_DOCUMENTS] = "Documents",
	[STORAGE_DIRECTORY_OTHERS] = "Others",
	[STORAGE_DIRECTORY_SYSTEM_RINGTONES] = "/opt/usr/share/settings/Ringtones",
};

static dd_list *st_head;

void add_device(const struct storage_ops *st)
{
	DD_LIST_APPEND(st_head, st);
}

void remove_device(const struct storage_ops *st)
{
	DD_LIST_REMOVE(st_head, st);
}

API int storage_foreach_device_supported(storage_device_supported_cb callback, void *user_data)
{
	const struct storage_ops *st;
	dd_list *elem;
	int storage_id = 0, ret;

	if (!callback) {
		_E("Invalid parameter");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	DD_LIST_FOREACH(st_head, elem, st) {
		ret = callback(storage_id, st->type, st->get_state(),
				st->root(), user_data);
		/* if the return value is false, will be stop to iterate */
		if (!ret)
			break;
		storage_id++;
	}

	return STORAGE_ERROR_NONE;
}

API int storage_get_root_directory(int storage_id, char **path)
{
	const struct storage_ops *st;

	if (!path) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	st = DD_LIST_NTH(st_head, storage_id);
	if (!st) {
		_E("Not supported storage : id(%d)", storage_id);
		return STORAGE_ERROR_NOT_SUPPORTED;
	}

	*path = strdup(st->root());
	if (!*path) {
		_E("Failed to copy the root string : %s", strerror(errno));
		return STORAGE_ERROR_OUT_OF_MEMORY;
	}

	return STORAGE_ERROR_NONE;
}

API int storage_get_directory(int storage_id, storage_directory_e type, char **path)
{
	const struct storage_ops *st;
	const char *root;
	char temp[PATH_MAX];

	if (!path) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	if (type < 0 || type >= STORAGE_DIRECTORY_MAX) {
		_E("Invalid parameter");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	st = DD_LIST_NTH(st_head, storage_id);
	if (!st) {
		_E("Not supported storage : id(%d)", storage_id);
		return STORAGE_ERROR_NOT_SUPPORTED;
	}

	if (st->type != STORAGE_TYPE_INTERNAL
	    && type == STORAGE_DIRECTORY_SYSTEM_RINGTONES) {
		_E("Not support directory : id(%d) type(%d)", storage_id, type);
		return STORAGE_ERROR_NOT_SUPPORTED;
	}

	root = st->root();
	if (type == STORAGE_DIRECTORY_SYSTEM_RINGTONES)
		snprintf(temp, PATH_MAX, "%s", dir_path[type]);
	else
		snprintf(temp, PATH_MAX, "%s/%s", root, dir_path[type]);

	*path = strdup(temp);
	if (!*path) {
		_E("Failed to copy the directory(%d) string : %s", type, strerror(errno));
		return STORAGE_ERROR_OUT_OF_MEMORY;
	}

	return STORAGE_ERROR_NONE;
}

API int storage_get_type(int storage_id, storage_type_e *type)
{
	const struct storage_ops *st;

	if (!type) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	st = DD_LIST_NTH(st_head, storage_id);
	if (!st) {
		_E("Not supported storage : id(%d)", storage_id);
		return STORAGE_ERROR_NOT_SUPPORTED;
	}

	*type = st->type;

	return STORAGE_ERROR_NONE;
}

API int storage_get_state(int storage_id, storage_state_e *state)
{
	const struct storage_ops *st;

	if (!state) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	st = DD_LIST_NTH(st_head, storage_id);
	if (!st) {
		_E("Not supported storage : id(%d)", storage_id);
		return STORAGE_ERROR_NOT_SUPPORTED;
	}

	*state = st->get_state();

	return STORAGE_ERROR_NONE;
}

API int storage_set_state_changed_cb(int storage_id, storage_state_changed_cb callback, void *user_data)
{
	const struct storage_ops *st;
	struct storage_cb_info info;
	int ret;

	if (!callback) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	st = DD_LIST_NTH(st_head, storage_id);
	if (!st) {
		_E("Not supported storage : id(%d)", storage_id);
		return STORAGE_ERROR_NOT_SUPPORTED;
	}

	/* do not register changed callback in case of internal memory */
	if (st->type == STORAGE_TYPE_INTERNAL)
		return STORAGE_ERROR_NONE;

	info.id = storage_id;
	info.state_cb = callback;
	info.user_data = user_data;

	ret = st->register_cb(STORAGE_CALLBACK_STATE, &info);
	if (ret < 0) {
		_E("Failed to register callback : id(%d)", storage_id);
		return STORAGE_ERROR_OPERATION_FAILED;
	}

	return STORAGE_ERROR_NONE;
}

API int storage_unset_state_changed_cb(int storage_id, storage_state_changed_cb callback)
{
	const struct storage_ops *st;
	struct storage_cb_info info;
	int ret;

	if (!callback) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	st = DD_LIST_NTH(st_head, storage_id);
	if (!st) {
		_E("Not supported storage : id(%d)", storage_id);
		return STORAGE_ERROR_NOT_SUPPORTED;
	}

	/* in case of internal memory, it does not register changed callback */
	if (st->type == STORAGE_TYPE_INTERNAL)
		return STORAGE_ERROR_NONE;

	info.id = storage_id;
	info.state_cb = callback;

	ret = st->unregister_cb(STORAGE_CALLBACK_STATE, &info);
	if (ret < 0) {
		_E("Failed to unregister callback : id(%d)", storage_id);
		return STORAGE_ERROR_OPERATION_FAILED;
	}

	return STORAGE_ERROR_NONE;
}

API int storage_get_total_space(int storage_id, unsigned long long *bytes)
{
	const struct storage_ops *st;
	unsigned long long total;
	int ret;

	if (!bytes) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	st = DD_LIST_NTH(st_head, storage_id);
	if (!st) {
		_E("Not supported storage : id(%d)", storage_id);
		return STORAGE_ERROR_NOT_SUPPORTED;
	}

	ret = st->get_space(&total, NULL);
	if (ret < 0) {
		_E("Failed to get total memory : id(%d)", storage_id);
		return STORAGE_ERROR_OPERATION_FAILED;
	}

	*bytes = total;

	return STORAGE_ERROR_NONE;
}

API int storage_get_available_space(int storage_id, unsigned long long *bytes)
{
	const struct storage_ops *st;
	unsigned long long avail;
	int ret;

	if (!bytes) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	st = DD_LIST_NTH(st_head, storage_id);
	if (!st) {
		_E("Not supported storage : id(%d)", storage_id);
		return STORAGE_ERROR_NOT_SUPPORTED;
	}

	ret = st->get_space(NULL, &avail);
	if (ret < 0) {
		_E("Failed to get available memory : id(%d)", storage_id);
		return STORAGE_ERROR_OPERATION_FAILED;
	}

	*bytes = avail;

	return STORAGE_ERROR_NONE;
}
