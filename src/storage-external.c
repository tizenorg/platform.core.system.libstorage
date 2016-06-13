/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd All Rights Reserved
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
#include <sys/statvfs.h>
#include <vconf.h>
#include <tzplatform_config.h>

#include "common.h"
#include "list.h"
#include "log.h"
#include "storage-external-dbus.h"

static dd_list *cb_list[STORAGE_CALLBACK_MAX];

static int storage_ext_get_dev_state(storage_ext_device *dev,
		enum storage_ext_state blk_state,
		storage_state_e *state)
{
	if (!dev || !state)
		return -EINVAL;

	switch (blk_state) {
	case STORAGE_EXT_ADDED:
		*state = STORAGE_STATE_UNMOUNTABLE;
		return 0;
	case STORAGE_EXT_REMOVED:
		*state = STORAGE_STATE_REMOVED;
		return 0;
	case STORAGE_EXT_CHANGED:
		switch (dev->state) {
		case STORAGE_EXT_UNMOUNTED:
			*state = STORAGE_STATE_UNMOUNTABLE;
			return 0;
		case STORAGE_EXT_MOUNTED:
			if (dev->flags & MOUNT_READONLY)
				*state = STORAGE_STATE_MOUNTED_READ_ONLY;
			else
				*state = STORAGE_STATE_MOUNTED;
			return 0;
		default:
			return -EINVAL;
		}
	default:
		return -EINVAL;
	}
}

int storage_ext_get_space(int storage_id,
		unsigned long long *total, unsigned long long *available)
{
	storage_state_e state;
	struct statvfs s;
	int ret;
	unsigned long long t = 0, a = 0;
	storage_ext_device *dev;

	if (storage_id < 0)
		return -ENOTSUP;

	dev = calloc(1, sizeof(storage_ext_device));
	if (!dev) {
//LCOV_EXCL_START System Error
		_E("calloc failed");
		return -ENOMEM;
//LCOV_EXCL_STOP
	}

	ret = storage_ext_get_device_info(storage_id, dev);
	if (ret < 0) {
		_E("Cannot get the storage with id (%d, ret:%d)", storage_id, ret); //LCOV_EXCL_LINE
		goto out;
	}

	ret = storage_ext_get_dev_state(dev, STORAGE_EXT_CHANGED, &state);
	if (ret < 0) {
		_E("Failed to get state of storage (id:%d, ret:%d)", storage_id, ret); //LCOV_EXCL_LINE
		goto out;
	}

	if (state >= STORAGE_STATE_MOUNTED) {
#ifndef __USE_FILE_OFFSET64
		ret = storage_get_external_memory_size_with_path(dev->mount_point, &s);
#else
		ret = storage_get_external_memory_size64_with_path(dev->mount_point, &s);
#endif
		if (ret < 0) {
			_E("Failed to get external memory size of (%s)(ret:%d)", dev->mount_point, ret); //LCOV_EXCL_LINE
			goto out;
		}

		t = (unsigned long long)s.f_frsize*s.f_blocks;
		a = (unsigned long long)s.f_bsize*s.f_bavail;
	}

	if (total)
		*total = t;
	if (available)
		*available = a;

	ret = 0;
out:
	storage_ext_release_device(&dev);
	return ret;
}

int storage_ext_foreach_device_list(storage_device_supported_cb callback, void *user_data)
{
	int ret;
	bool ret_cb;
	dd_list *list = NULL, *elem;
	storage_ext_device *dev;
	storage_state_e state;

	if (!callback)
		return -EINVAL;

	ret = storage_ext_get_list(&list);
	if (ret < 0) {
		_E("Failed to get external storage list from deviced (%d)", errno); //LCOV_EXCL_LINE
		return ret;
	}

	DD_LIST_FOREACH(list, elem, dev) {
		ret = storage_ext_get_dev_state(dev, STORAGE_EXT_CHANGED, &state);
		if (ret < 0) {
			_E("Failed to get storage state (devnode:%s, ret:%d)", dev->devnode, ret); //LCOV_EXCL_LINE
			continue;
		}

		ret_cb = callback(dev->storage_id, STORAGE_TYPE_EXTERNAL,
				state, dev->mount_point, user_data);
		if (!ret_cb)
			break;
	}

	if (list)
		storage_ext_release_list(&list);
	return 0;
}

//LCOV_EXCL_START Not called Callback
static int storage_ext_id_changed(storage_ext_device *dev, enum storage_ext_state blk_state, void *data)
{
	enum storage_cb_type type = (enum storage_cb_type)data;
	struct storage_cb_info *cb_info;
	dd_list *elem;
	storage_state_e state;
	int ret;

	if (!dev)
		return -EINVAL;

	if (type != STORAGE_CALLBACK_ID)
		return 0;

	ret = storage_ext_get_dev_state(dev, blk_state, &state);
	if (ret < 0) {
		_E("Failed to get storage state (devnode:%s, ret:%d)", dev->devnode, ret);
		return ret;
	}

	DD_LIST_FOREACH(cb_list[STORAGE_CALLBACK_ID], elem, cb_info)
		cb_info->state_cb(cb_info->id, state, cb_info->user_data);

	return 0;
}

static int storage_ext_type_changed(storage_ext_device *dev, enum storage_ext_state blk_state, void *data)
{
	enum storage_cb_type type = (enum storage_cb_type)data;
	struct storage_cb_info *cb_info;
	dd_list *elem;
	storage_state_e state;
	int ret;
	storage_dev_e strdev;
	const char *fstype, *fsuuid, *mountpath;

	if (!dev)
		return -EINVAL;

	if (type != STORAGE_CALLBACK_TYPE)
		return -EINVAL;

	ret = storage_ext_get_dev_state(dev, blk_state, &state);
	if (ret < 0) {
		_E("Failed to get storage state (devnode:%s, ret:%d)", dev->devnode, ret);
		return ret;
	}

	if (dev->type == STORAGE_EXT_SCSI)
		strdev = STORAGE_DEV_EXT_USB_MASS_STORAGE;
	else if (dev->type == STORAGE_EXT_MMC)
		strdev = STORAGE_DEV_EXT_SDCARD;
	else {
		_E("Invalid dev type (%d)", dev->type);
		return -EINVAL;
	}

	fstype = (dev->fs_type ? (const char *)dev->fs_type : "");
	fsuuid = (dev->fs_uuid ? (const char *)dev->fs_uuid : "");
	mountpath = (dev->mount_point ? (const char *)dev->mount_point : "");

	DD_LIST_FOREACH(cb_list[STORAGE_CALLBACK_TYPE], elem, cb_info)
		if (cb_info->type_cb)
			cb_info->type_cb(dev->storage_id, strdev, state,
					fstype, fsuuid, mountpath, dev->primary,
					dev->flags, cb_info->user_data);

	return 0;
}

//LCOV_EXCL_STOP

static bool check_if_callback_exist(enum storage_cb_type type,
		struct storage_cb_info *info, struct storage_cb_info **cb_data)
{
	struct storage_cb_info *cb_info;
	dd_list *elem;

	if (!info)
		return false;

	if (type == STORAGE_CALLBACK_ID) {
		DD_LIST_FOREACH(cb_list[type], elem, cb_info) {
			if (cb_info->id == info->id &&
			    cb_info->state_cb == info->state_cb) {
				goto out;
			}
		}
	}

	if (type == STORAGE_CALLBACK_TYPE) {
		DD_LIST_FOREACH(cb_list[type], elem, cb_info) {
			if (cb_info->type == info->type &&
			    cb_info->type_cb == info->type_cb)
				goto out;
		}
	}

	return false;

out:
	if (cb_data)
		*cb_data = cb_info;

	return true;
}

int storage_ext_register_cb(enum storage_cb_type type, struct storage_cb_info *info)
{
	struct storage_cb_info *cb_info;
	int n, ret;
	storage_ext_changed_cb callback;

	if (!info)
		return -EINVAL;

	switch (type) {
	case STORAGE_CALLBACK_ID:
		callback = storage_ext_id_changed;
		break;
	case STORAGE_CALLBACK_TYPE:
		callback = storage_ext_type_changed;
		break;
	default:
		_E("Invalid callback type (%d)", type);
		return -EINVAL;
	}

	n = DD_LIST_LENGTH(cb_list[type]);
	if (n == 0) {
		ret = storage_ext_register_device_change(callback, (void *)type);
		if (ret < 0)
			return -EPERM;
	}

	if (check_if_callback_exist(type, info, NULL)) {
		_E("The callback is already registered");
		return 0;
	}

	/* add device changed callback to list (local) */
	cb_info = malloc(sizeof(struct storage_cb_info));
	if (!cb_info)
		return -errno;

	memcpy(cb_info, info, sizeof(struct storage_cb_info));
	DD_LIST_APPEND(cb_list[type], cb_info);

	return 0;
}

int storage_ext_unregister_cb(enum storage_cb_type type, struct storage_cb_info *info)
{
	struct storage_cb_info *cb_info;
	int n;
	storage_ext_changed_cb callback;

	if (!info)
		return -EINVAL;

	switch (type) {
	case STORAGE_CALLBACK_ID:
		callback = storage_ext_id_changed;
		break;
	case STORAGE_CALLBACK_TYPE:
		callback = storage_ext_type_changed;
		break;
	default:
		_E("Invalid callback type (%d)", type);
		return -EINVAL;
	}

	if (!check_if_callback_exist(type, info, &cb_info)) {
		_E("The callback is not registered");
		return 0;
	}

	/* remove device callback from list (local) */
	if (cb_info) {
		DD_LIST_REMOVE(cb_list[type], cb_info);
		free(cb_info);
	}

	/* check if this callback is last element */
	n = DD_LIST_LENGTH(cb_list[type]);
	if (n == 0)
		storage_ext_unregister_device_change(callback);

	return 0;
}

int storage_ext_get_root(int storage_id, char *path, size_t len)
{
	storage_ext_device *dev;
	int ret;

	if (storage_id < 0)
		return -ENOTSUP;

	if (!path)
		return -EINVAL;

	dev = calloc(1, sizeof(storage_ext_device));
	if (!dev) {
//LCOV_EXCL_START System Error
		_E("calloc failed");
		return -ENOMEM;
//LCOV_EXCL_STOP
	}

	ret = storage_ext_get_device_info(storage_id, dev);
	if (ret < 0) {
		_E("Cannot get the storage with id (%d, ret:%d)", storage_id, ret); //LCOV_EXCL_LINE
		goto out;
	}

	snprintf(path, len, "%s", dev->mount_point);
	ret = 0;

out:
	storage_ext_release_device(&dev);
	return ret;
}

int storage_ext_get_state(int storage_id, storage_state_e *state)
{
	storage_ext_device *dev;
	int ret;

	if (storage_id < 0)
		return -ENOTSUP;

	if (!state)
		return -EINVAL;

	dev = calloc(1, sizeof(storage_ext_device));
	if (!dev) {
//LCOV_EXCL_START System Error
		_E("calloc failed");
		return -ENOMEM;
//LCOV_EXCL_STOP
	}

	ret = storage_ext_get_device_info(storage_id, dev);
	if (ret < 0) {
		_E("Cannot get the storage with id (%d, ret:%d)", storage_id, ret); //LCOV_EXCL_LINE
		goto out;
	}

	ret = storage_ext_get_dev_state(dev, STORAGE_EXT_CHANGED, state);
	if (ret < 0)
		_E("Failed to get state of storage id (%d, ret:%d)", storage_id, ret); //LCOV_EXCL_LINE

out:
	storage_ext_release_device(&dev);
	return ret;
}

int storage_ext_get_primary_mmc_path(char *path, size_t len)
{
	dd_list *list = NULL, *elem;
	storage_ext_device *dev;
	int ret;

	ret = storage_ext_get_list(&list);
	if (ret < 0) {
		_E("Failed to get external storage list from deviced (%d)", errno); //LCOV_EXCL_LINE
		return ret;
	}

	DD_LIST_FOREACH(list, elem, dev) {
		if (dev->primary) {
			snprintf(path, len, "%s", dev->mount_point);
			ret = 0;
			goto out;
		}
	}

	ret = -ENODEV;

out:
	if (list)
		storage_ext_release_list(&list);
	return ret;
}
