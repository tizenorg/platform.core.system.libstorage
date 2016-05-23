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
		_E("calloc failed");
		return -ENOMEM;
	}

	ret = storage_ext_get_device_info(storage_id, dev);
	if (ret < 0) {
		_E("Cannot get the storage with id (%d, ret:%d)", storage_id, ret);
		goto out;
	}

	ret = storage_ext_get_dev_state(dev, STORAGE_EXT_CHANGED, &state);
	if (ret < 0) {
		_E("Failed to get state of storage (id:%d, ret:%d)", storage_id, ret);
		goto out;
	}

	if (state >= STORAGE_STATE_MOUNTED) {
#ifndef __USE_FILE_OFFSET64
		ret = storage_get_external_memory_size_with_path(dev->mount_point, &s);
#else
		ret = storage_get_external_memory_size64_with_path(dev->mount_point, &s);
#endif
		if (ret < 0) {
			_E("Failed to get external memory size of (%s)(ret:%d)", dev->mount_point, ret);
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
		_E("Failed to get external storage list from deviced (%d)", errno);
		return ret;
	}

	DD_LIST_FOREACH(list, elem, dev) {
		ret = storage_ext_get_dev_state(dev, STORAGE_EXT_CHANGED, &state);
		if (ret < 0) {
			_E("Failed to get storage state (devnode:%s, ret:%d)", dev->devnode, ret);
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

static int storage_ext_state_changed(storage_ext_device *dev, enum storage_ext_state blk_state, void *data)
{
	enum storage_cb_type type = (enum storage_cb_type)data;
	struct storage_cb_info *cb_info;
	dd_list *elem;
	storage_state_e state;
	int ret;

	if (!dev)
		return -EINVAL;

	if (type != STORAGE_CALLBACK_STATE)
		return 0;

	ret = storage_ext_get_dev_state(dev, blk_state, &state);
	if (ret < 0) {
		_E("Failed to get storage state (devnode:%s, ret:%d)", dev->devnode, ret);
		return ret;
	}

	DD_LIST_FOREACH(cb_list[STORAGE_CALLBACK_STATE], elem, cb_info)
		cb_info->state_cb(cb_info->id, state, cb_info->user_data);

	return 0;
}

int storage_ext_register_cb(enum storage_cb_type type, struct storage_cb_info *info)
{
	struct storage_cb_info *cb_info;
	dd_list *elem;
	int ret, n;

	if (type < 0 || type >= STORAGE_CALLBACK_MAX)
		return -EINVAL;

	if (!info)
		return -EINVAL;

	/* check if it is the first request */
	n = DD_LIST_LENGTH(cb_list[type]);
	if (n == 0) {
		ret = storage_ext_register_device_change(storage_ext_state_changed, (void *)type);
		if (ret < 0)
			return -EPERM;
	}

	/* check for the same request */
	DD_LIST_FOREACH(cb_list[type], elem, cb_info) {
		if (cb_info->id == info->id &&
		    cb_info->state_cb == info->state_cb)
			return -EEXIST;
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
	dd_list *elem;
	int n;

	if (type < 0 || type >= STORAGE_CALLBACK_MAX)
		return -EINVAL;

	if (!info)
		return -EINVAL;

	/* search for the same element with callback */
	DD_LIST_FOREACH(cb_list[type], elem, cb_info) {
		if (cb_info->id == info->id &&
		    cb_info->state_cb == info->state_cb)
			break;
	}

	if (!cb_info)
		return -EINVAL;

	/* remove device callback from list (local) */
	DD_LIST_REMOVE(cb_list[type], cb_info);
	free(cb_info);

	/* check if this callback is last element */
	n = DD_LIST_LENGTH(cb_list[type]);
	if (n == 0)
		storage_ext_unregister_device_change(storage_ext_state_changed);

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
		_E("calloc failed");
		return -ENOMEM;
	}

	ret = storage_ext_get_device_info(storage_id, dev);
	if (ret < 0) {
		_E("Cannot get the storage with id (%d, ret:%d)", storage_id, ret);
		goto out;
	}

	snprintf(path, len, "%s", dev->mount_point);
	ret = 0;

out:
	storage_ext_release_device(&dev);
	return ret;
}

int storage_ext_get_type(int storage_id, storage_type_e *type)
{
	storage_ext_device *dev = NULL;
	int ret;

	if (storage_id < 0)
		return -ENOTSUP;

	if (!path)
		return -EINVAL;

	dev = calloc(1, sizeof(storage_ext_device));
	if (!dev) {
		_E("calloc failed");
		return -ENOMEM;
	}

	ret = storage_ext_get_device_info(storage_id, dev);
	if (ret < 0) {
		_E("Cannot get the storage with id (%d, ret:%d)", storage_id, ret);
		return ret;
	}

	storage_ext_release_device(&dev);
	*type = STORAGE_TYPE_EXTERNAL;
	return 0;
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
		_E("calloc failed");
		return -ENOMEM;
	}

	ret = storage_ext_get_device_info(storage_id, dev);
	if (ret < 0) {
		_E("Cannot get the storage with id (%d, ret:%d)", storage_id, ret);
		goto out;
	}

	ret = storage_ext_get_dev_state(dev, STORAGE_EXT_CHANGED, state);
	if (ret < 0)
		_E("Failed to get state of storage id (%d, ret:%d)", storage_id, ret);

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
		_E("Failed to get external storage list from deviced (%d)", errno);
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
