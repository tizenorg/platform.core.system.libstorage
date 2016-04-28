/*
 * libstorage
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __STORAGE_EXTERNAL_DBUS_H__
#define __STORAGE_EXTERNAL_DBUS_H__

#include <stdbool.h>
#include <glib.h>
#include <gio/gio.h>
#include "list.h"

#define STORAGE_EXT_BUS_NAME              "org.tizen.system.deviced"
#define STORAGE_EXT_PATH                  "/Org/Tizen/System/DeviceD/Block"
#define STORAGE_EXT_PATH_DEVICES          STORAGE_EXT_PATH"/Devices"
#define STORAGE_EXT_PATH_MANAGER          STORAGE_EXT_PATH"/Manager"
#define STORAGE_EXT_IFACE                 STORAGE_EXT_BUS_NAME".Block"
#define STORAGE_EXT_IFACE_MANAGER         STORAGE_EXT_BUS_NAME".BlockManager"

enum mount_state {
	STORAGE_EXT_UNMOUNTED,
	STORAGE_EXT_MOUNTED,
};

enum storage_ext_state {
	STORAGE_EXT_REMOVED,
	STORAGE_EXT_ADDED,
	STORAGE_EXT_CHANGED,
};

enum storage_ext_type {
	STORAGE_EXT_SCSI,
	STORAGE_EXT_MMC,
};

enum storage_ext_flags {
	FLAG_NONE        = 0,
	UNMOUNT_UNSAFE   = 1 << 0,
	FS_BROKEN        = 1 << 1,
	FS_EMPTY         = 1 << 2,
	FS_NOT_SUPPORTED = 1 << 3,
	MOUNT_READONLY   = 1 << 4,
};

typedef struct _storage_ext_device {
	enum storage_ext_type type;
	char *devnode;
	char *syspath;
	char *fs_usage;
	char *fs_type;
	char *fs_version;
	char *fs_uuid;
	bool readonly;
	char *mount_point;
	enum mount_state state;
	bool primary;   /* the first partition */
	int flags;
	int storage_id;
} storage_ext_device;

typedef int (*storage_ext_changed_cb)(storage_ext_device *dev, enum storage_ext_state state, void *data);

void storage_ext_release_device(storage_ext_device **dev);
void storage_ext_release_list(dd_list **list);
int storage_ext_get_list(dd_list **list);

int storage_ext_register_device_change(storage_ext_changed_cb func, void *data);
void storage_ext_unregister_device_change(storage_ext_changed_cb func);

int storage_ext_get_device_info(int storage_id, storage_ext_device *info);

#endif /* __STORAGE_EXTERNAL_DBUS_H__ */
