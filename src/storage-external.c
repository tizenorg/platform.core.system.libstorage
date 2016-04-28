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