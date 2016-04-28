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
#include <system_settings.h>

#include "common.h"
#include "list.h"
#include "log.h"
#include "storage-external.h"

const char *dir_path[STORAGE_DIRECTORY_MAX] = {
	[STORAGE_DIRECTORY_IMAGES] = "Images",
	[STORAGE_DIRECTORY_SOUNDS] = "Sounds",
	[STORAGE_DIRECTORY_VIDEOS] = "Videos",
	[STORAGE_DIRECTORY_CAMERA] = "Camera",
	[STORAGE_DIRECTORY_DOWNLOADS] = "Downloads",
	[STORAGE_DIRECTORY_MUSIC] = "Music",
	[STORAGE_DIRECTORY_DOCUMENTS] = "Documents",
	[STORAGE_DIRECTORY_OTHERS] = "Others",
	[STORAGE_DIRECTORY_SYSTEM_RINGTONES] = "",
};

static dd_list *st_int_head; /* Internal storage list */

void add_device(const struct storage_ops *st)
{
	DD_LIST_APPEND(st_int_head, st);
}

void remove_device(const struct storage_ops *st)
{
	DD_LIST_REMOVE(st_int_head, st);
}

API int storage_foreach_device_supported(storage_device_supported_cb callback, void *user_data)
{
	const struct storage_ops *st;
	dd_list *elem;
	int ret;

	if (!callback) {
		_E("Invalid parameter");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	DD_LIST_FOREACH(st_int_head, elem, st) {
		ret = callback(st->storage_id, st->type, st->get_state(),
				st->root(), user_data);
		/* if the return value is false, will be stop to iterate */
		if (!ret)
			break;
	}

	ret = storage_ext_foreach_device_list(callback, user_data);
	if (ret < 0) {
		_E("Failed to iterate external devices (%d)", ret);
		return STORAGE_ERROR_OPERATION_FAILED;
	}

	return STORAGE_ERROR_NONE;
}

API int storage_get_root_directory(int storage_id, char **path)
{
	const struct storage_ops *st;
	dd_list *elem;

	if (!path || storage_id < 0) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	/* internal storage */
	DD_LIST_FOREACH(st_int_head, elem, st) {
		if (st->storage_id != storage_id)
			continue;
		*path = strdup(st->root());
		if (!*path) {
			_E("Failed to copy the root string : %d", errno);
			return STORAGE_ERROR_OUT_OF_MEMORY;
		}
		return STORAGE_ERROR_NONE;
	}

	/* TODO external storage */

	return STORAGE_ERROR_NONE;
}

API int storage_get_directory(int storage_id, storage_directory_e type, char **path)
{
	const struct storage_ops *st;
	char root[PATH_MAX];
	char temp[PATH_MAX];
	char *temp2, *end;
	int ret;
	dd_list *elem;
	bool found;

	if (!path || storage_id < 0) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	if (type < 0 || type >= STORAGE_DIRECTORY_MAX) {
		_E("Invalid parameter");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	/* internal storage */
	found = false;
	DD_LIST_FOREACH(st_int_head, elem, st) {
		if (st->storage_id != storage_id)
			continue;
		found = true;
		break;
	}

	if (found && st) {
		snprintf(root, sizeof(root), "%s", st->root());
		if (type == STORAGE_DIRECTORY_SYSTEM_RINGTONES) {
			ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_INCOMING_CALL_RINGTONE, &temp2);
			if (ret < 0) {
				_E("Failed to get ringtone path : %d", ret);
				return STORAGE_ERROR_OPERATION_FAILED;
			}
			end = strrchr(temp2, '/');
			if (end)
				*end = '\0';
			snprintf(temp, PATH_MAX, "%s", temp2);
			free(temp2);
		} else
			snprintf(temp, PATH_MAX, "%s/%s", root, dir_path[type]);

		goto out;
	}

	/* external storage */
	return STORAGE_ERROR_NONE;

out:
	*path = strdup(temp);
	if (!*path) {
		_E("Failed to copy the directory(%d) string : %d", type, errno);
		return STORAGE_ERROR_OUT_OF_MEMORY;
	}

	return STORAGE_ERROR_NONE;
}

API int storage_get_type(int storage_id, storage_type_e *type)
{
	const struct storage_ops *st;
	dd_list *elem;

	if (!type || storage_id < 0) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	/* internal storage */
	DD_LIST_FOREACH(st_int_head, elem, st) {
		if (st->storage_id != storage_id)
			continue;
		*type = st->type;
		return STORAGE_ERROR_NONE;
	}

	/* external storage */

	return STORAGE_ERROR_NONE;
}

API int storage_get_state(int storage_id, storage_state_e *state)
{
	const struct storage_ops *ops;
	dd_list *elem;

	if (!state) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	/* internal storage */
	DD_LIST_FOREACH(st_int_head, elem, ops) {
		if (ops->storage_id != storage_id)
			continue;
		*state = ops->get_state();
		return STORAGE_ERROR_NONE;
	}

	/* external storage */

	return STORAGE_ERROR_NONE;
}

API int storage_set_state_changed_cb(int storage_id, storage_state_changed_cb callback, void *user_data)
{
	const struct storage_ops *st;
	dd_list *elem;

	if (!callback) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	/* Internal storage does not support registering changed callback */
	DD_LIST_FOREACH(st_int_head, elem, st)
		if (st->storage_id == storage_id)
			return STORAGE_ERROR_NONE;

	/* external storage */

	return STORAGE_ERROR_NONE;
}

API int storage_unset_state_changed_cb(int storage_id, storage_state_changed_cb callback)
{
	const struct storage_ops *st;
	dd_list *elem;

	if (!callback) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	/* Internal storage does not support registering changed callback */
	DD_LIST_FOREACH(st_int_head, elem, st)
		if (st->storage_id == storage_id)
			return STORAGE_ERROR_NONE;

	return STORAGE_ERROR_NONE;
}

API int storage_get_total_space(int storage_id, unsigned long long *bytes)
{
	const struct storage_ops *st;
	unsigned long long total;
	int ret;
	dd_list *elem;

	if (!bytes || storage_id < 0) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	/* internal storage */
	DD_LIST_FOREACH(st_int_head, elem, st) {
		if (st->storage_id != storage_id)
			continue;
		ret = st->get_space(&total, NULL);
		goto out;
	}

	/* external storage */
	ret = 0;

out:
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
	dd_list *elem;

	if (!bytes) {
		_E("Invalid parameger");
		return STORAGE_ERROR_INVALID_PARAMETER;
	}

	/* internal storage */
	DD_LIST_FOREACH(st_int_head, elem, st) {
		if (st->storage_id != storage_id)
			continue;
		ret = st->get_space(NULL, &avail);
		goto out;
	}

	/* external storage */
	ret = 0;

out:
	if (ret < 0) {
		_E("Failed to get available memory : id(%d)", storage_id);
		return STORAGE_ERROR_OPERATION_FAILED;
	}

	*bytes = avail;
	return STORAGE_ERROR_NONE;
}
