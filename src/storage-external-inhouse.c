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

#include "common.h"
#include "list.h"
#include "log.h"
#include "storage-internal.h"
#include "storage-external-dbus.h"

API int storage_get_primary_sdcard(int *storage_id, char **path)
{
	GVariant *result;
	storage_ext_device info;

	if (!storage_id || !path)
		return STORAGE_ERROR_INVALID_PARAMETER;

	result = dbus_method_call_sync(STORAGE_EXT_BUS_NAME,
			STORAGE_EXT_PATH_MANAGER,
			STORAGE_EXT_IFACE_MANAGER,
			"GetMmcPrimary",
			NULL);
	if (!result) {
		_E("Failed to get primary sdcard partition"); //LCOV_EXCL_LINE
		return STORAGE_ERROR_OPERATION_FAILED; //LCOV_EXCL_LINE
	}

	g_variant_get(result, "(issssssisibii)",
			&info.type, &info.devnode, &info.syspath,
			&info.fs_usage, &info.fs_type,
			&info.fs_version, &info.fs_uuid,
			&info.readonly, &info.mount_point,
			&info.state, &info.primary,
			&info.flags, &info.storage_id);

	g_variant_unref(result);

	if (info.storage_id < 0)
		return STORAGE_ERROR_NO_DEVICE;

	*path = strdup(info.mount_point);
	if (*path == NULL)
		return STORAGE_ERROR_OUT_OF_MEMORY;

	*storage_id = info.storage_id;

	return STORAGE_ERROR_NONE;
}
