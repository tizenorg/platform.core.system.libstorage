/*
 * libstorage
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <vconf.h>
#include <stdbool.h>
#include <errno.h>
#include <gio/gio.h>
#include <glib.h>
#include <limits.h>

#include "log.h"
#include "storage-external-dbus.h"

#define CHECK_STR(a) (a ? a : "")

#define STORAGE_EXT_GET_LIST       "GetDeviceList"

#define STORAGE_EXT_OBJECT_ADDED   "ObjectAdded"
#define STORAGE_EXT_OBJECT_REMOVED "ObjectRemoved"
#define STORAGE_EXT_DEVICE_CHANGED "DeviceChanged"

#define DBUS_REPLY_TIMEOUT (-1)

#define DEV_PREFIX           "/dev/"

struct storage_ext_callback {
	storage_ext_changed_cb func;
	void *data;
	guint block_id;
	guint blockmanager_id;
};

static dd_list *changed_list;

static void storage_ext_release_internal(storage_ext_device *dev)
{
	if (!dev)
		return;
	free(dev->devnode);
	free(dev->syspath);
	free(dev->fs_usage);
	free(dev->fs_type);
	free(dev->fs_version);
	free(dev->fs_uuid);
	free(dev->mount_point);
}

void storage_ext_release_device(storage_ext_device **dev)
{
	if (!dev || !*dev)
		return;
	storage_ext_release_internal(*dev);
	free(*dev);
	*dev = NULL;
}

void storage_ext_release_list(dd_list **list)
{
	storage_ext_device *dev;
	dd_list *elem;

	if (*list == NULL)
		return;

	DD_LIST_FOREACH(*list, elem, dev) {
		storage_ext_release_internal(dev);
		free(dev);
	}

	g_list_free(*list);
	*list = NULL;
}

static GDBusConnection *get_dbus_connection(void)
{
	GError *err = NULL;
	static GDBusConnection *conn;

	if (conn)
		return conn;

#if !GLIB_CHECK_VERSION(2, 35, 0)
	g_type_init();
#endif

	conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
	if (!conn) {
//LCOV_EXCL_START System Error
		if (err) {
			_E("fail to get dbus connection : %s", err->message);
			g_clear_error(&err);
		} else
			_E("fail to get dbus connection");
		return NULL;
//LCOV_EXCL_STOP
	}
	return conn;
}

static GVariant *dbus_method_call_sync(const gchar *dest, const gchar *path,
				const gchar *iface, const gchar *method, GVariant *param)
{
	GDBusConnection *conn;
	GError *err = NULL;
	GVariant *ret;

	if (!dest || !path || !iface || !method || !param)
		return NULL;

	conn = get_dbus_connection();
	if (!conn) {
		_E("fail to get dbus connection"); //LCOV_EXCL_LINE
		return NULL;
	}

	ret = g_dbus_connection_call_sync(conn,
			dest, path, iface, method,
			param, NULL, G_DBUS_CALL_FLAGS_NONE,
			-1, NULL, &err);
	if (!ret) {
//LCOV_EXCL_START System Error
		if (err) {
			_E("dbus method sync call failed(%s)", err->message);
			g_clear_error(&err);
		} else
			_E("g_dbus_connection_call_sync() failed");
		return NULL;
//LCOV_EXCL_STOP
	}

	return ret;
}

int storage_ext_get_list(dd_list **list)
{
	GVariant *result;
	GVariantIter *iter;
	storage_ext_device *elem, info;
	int ret;

	if (!list)
		return -EINVAL;

	result = dbus_method_call_sync(STORAGE_EXT_BUS_NAME,
			STORAGE_EXT_PATH_MANAGER,
			STORAGE_EXT_IFACE_MANAGER,
			STORAGE_EXT_GET_LIST,
			g_variant_new("(s)", "all"));
	if (!result) {
		_E("Failed to get storage_ext device info"); //LCOV_EXCL_LINE
		return -EIO;
	}

	g_variant_get(result, "(a(issssssisibii))", &iter);

	while (g_variant_iter_loop(iter, "(issssssisibii)",
				&info.type, &info.devnode, &info.syspath,
				&info.fs_usage, &info.fs_type,
				&info.fs_version, &info.fs_uuid,
				&info.readonly, &info.mount_point,
				&info.state, &info.primary,
				&info.flags, &info.storage_id)) {

		elem = (storage_ext_device *)malloc(sizeof(storage_ext_device));
		if (!elem) {
			_E("malloc() failed"); //LCOV_EXCL_LINE
			ret = -ENOMEM;
			goto out;
		}

		elem->type = info.type;
		elem->readonly = info.readonly;
		elem->state = info.state;
		elem->primary = info.primary;
		elem->devnode = strdup(CHECK_STR(info.devnode));
		elem->syspath = strdup(CHECK_STR(info.syspath));
		elem->fs_usage = strdup(CHECK_STR(info.fs_usage));
		elem->fs_type = strdup(CHECK_STR(info.fs_type));
		elem->fs_version = strdup(CHECK_STR(info.fs_version));
		elem->fs_uuid = strdup(CHECK_STR(info.fs_uuid));
		elem->mount_point = strdup(CHECK_STR(info.mount_point));
		elem->flags = info.flags;
		elem->storage_id = info.storage_id;

		DD_LIST_APPEND(*list, elem);
	}

	ret = g_list_length(*list);

out:
	if (ret < 0)
		storage_ext_release_list(list);
	g_variant_iter_free(iter);
	g_variant_unref(result);
	return ret;
}

//LCOV_EXCL_START Not called Callback
static char *get_devnode_from_path(char *path)
{
	if (!path)
		return NULL;
	/* 1 means '/' */
	return path + strlen(STORAGE_EXT_PATH_DEVICES) + 1;
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START Not called Callback
static void storage_ext_object_path_changed(enum storage_ext_state state,
		GVariant *params, gpointer user_data)
{
	storage_ext_device *dev = NULL;
	dd_list *elem;
	struct storage_ext_callback *callback;
	char *path = NULL;
	char *devnode;
	int ret;

	if (!params)
		return;

	g_variant_get(params, "(s)", &path);

	devnode = get_devnode_from_path(path);
	if (!devnode)
		goto out;

	dev = calloc(1, sizeof(storage_ext_device));
	if (!dev)
		goto out;

	dev->devnode = strdup(devnode);
	if (dev->devnode == NULL) {
		_E("strdup() failed");
		goto out;
	}

	DD_LIST_FOREACH(changed_list, elem, callback) {
		if (!callback->func)
			continue;
		ret = callback->func(dev, state, callback->data);
		if (ret < 0)
			_E("Failed to call callback for devnode(%s, %d)", devnode, ret);
	}

out:
	if (dev) {
		free(dev->devnode);
		free(dev);
	}
	free(path);
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START Not called Callback
static void storage_ext_device_added(GVariant *params, gpointer user_data)
{
	storage_ext_object_path_changed(STORAGE_EXT_ADDED, params, user_data);
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START Not called Callback
static void storage_ext_device_removed(GVariant *params, gpointer user_data)
{
	storage_ext_object_path_changed(STORAGE_EXT_REMOVED, params, user_data);
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START Not called Callback
static void storage_ext_device_changed(GVariant *params, gpointer user_data)
{
	storage_ext_device *dev;
	dd_list *elem;
	struct storage_ext_callback *callback;
	int ret;

	if (!params)
		return;

	dev = calloc(1, sizeof(storage_ext_device));
	if (!dev)
		return;

	g_variant_get(params, "(issssssisibii)",
			&dev->type,
			&dev->devnode,
			&dev->syspath,
			&dev->fs_usage,
			&dev->fs_type,
			&dev->fs_version,
			&dev->fs_uuid,
			&dev->readonly,
			&dev->mount_point,
			&dev->state,
			&dev->primary,
			&dev->flags,
			&dev->storage_id);

	DD_LIST_FOREACH(changed_list, elem, callback) {
		if (!callback->func)
			continue;
		ret = callback->func(dev, STORAGE_EXT_CHANGED, callback->data);
		if (ret < 0)
			_E("Failed to call callback for devnode(%s, %d)", dev->devnode, ret);
	}

	storage_ext_release_device(&dev);
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START Not called Callback
static void storage_ext_changed(GDBusConnection *conn,
		const gchar *sender,
		const gchar *path,
		const gchar *iface,
		const gchar *signal,
		GVariant *params,
		gpointer user_data)
{
	size_t iface_len, signal_len;

	if (!params || !sender || !path || !iface || !signal)
		return;

	iface_len = strlen(iface) + 1;
	signal_len = strlen(signal) + 1;

	if (!strncmp(iface, STORAGE_EXT_IFACE_MANAGER, iface_len)) {
		if (!strncmp(signal, STORAGE_EXT_OBJECT_ADDED, signal_len))
			storage_ext_device_added(params, user_data);
		else if (!strncmp(signal, STORAGE_EXT_OBJECT_REMOVED, signal_len))
			storage_ext_device_removed(params, user_data);
		return;
	}

	if (!strncmp(iface, STORAGE_EXT_IFACE, iface_len) &&
		!strncmp(signal, STORAGE_EXT_DEVICE_CHANGED, signal_len)) {
		storage_ext_device_changed(params, user_data);
		return;
	}
}
//LCOV_EXCL_STOP

int storage_ext_register_device_change(storage_ext_changed_cb func, void *data)
{
	GDBusConnection *conn;
	guint block_id = NULL, blockmanager_id = NULL;
	struct storage_ext_callback *callback;
	dd_list *elem;

	if (!func)
		return -EINVAL;

	DD_LIST_FOREACH(changed_list, elem, callback) {
		if (callback->func != func)
			continue;
		if (callback->block_id == 0 || callback->blockmanager_id == 0)
			continue;

		return -EEXIST;
	}

	callback = (struct storage_ext_callback *)malloc(sizeof(struct storage_ext_callback));
	if (!callback) {
//LCOV_EXCL_START System Error
		_E("malloc() failed");
		return -ENOMEM;
//LCOV_EXCL_STOP
	}

	conn = get_dbus_connection();
	if (!conn) {
//LCOV_EXCL_START System Error
		free(callback);
		_E("Failed to get dbus connection");
		return -EPERM;
//LCOV_EXCL_STOP
	}

	block_id = g_dbus_connection_signal_subscribe(conn,
			STORAGE_EXT_BUS_NAME,
			STORAGE_EXT_IFACE,
			STORAGE_EXT_DEVICE_CHANGED,
			NULL,
			NULL,
			G_DBUS_SIGNAL_FLAGS_NONE,
			storage_ext_changed,
			NULL,
			NULL);
	if (block_id == 0) {
		free(callback);
		_E("Failed to subscrive bus signal");
		return -EPERM;
	}

	blockmanager_id = g_dbus_connection_signal_subscribe(conn,
			STORAGE_EXT_BUS_NAME,
			STORAGE_EXT_IFACE_MANAGER,
			NULL,
			STORAGE_EXT_PATH_MANAGER,
			NULL,
			G_DBUS_SIGNAL_FLAGS_NONE,
			storage_ext_changed,
			NULL,
			NULL);
	if (blockmanager_id == 0) {
//LCOV_EXCL_START System Error
		free(callback);
		_E("Failed to subscrive bus signal");
		return -EPERM;
//LCOV_EXCL_STOP
	}

	callback->func = func;
	callback->data = data;
	callback->block_id = block_id;
	callback->blockmanager_id = blockmanager_id;

	DD_LIST_APPEND(changed_list, callback);

	return 0;
}

void storage_ext_unregister_device_change(storage_ext_changed_cb func)
{
	GDBusConnection *conn;
	struct storage_ext_callback *callback;
	dd_list *elem;

	if (!func)
		return;

	conn = get_dbus_connection();
	if (!conn) {
//LCOV_EXCL_START System Error
		_E("fail to get dbus connection");
		return;
//LCOV_EXCL_STOP
	}

	DD_LIST_FOREACH(changed_list, elem, callback) {
		if (callback->func != func)
			continue;
		if (callback->block_id > 0)
			g_dbus_connection_signal_unsubscribe(conn, callback->block_id);
		if (callback->blockmanager_id > 0)
			g_dbus_connection_signal_unsubscribe(conn, callback->blockmanager_id);

		DD_LIST_REMOVE(changed_list, callback);
		free(callback);
	}
}

int storage_ext_get_device_info(int storage_id, storage_ext_device *info)
{
	GVariant *result;

	result = dbus_method_call_sync(STORAGE_EXT_BUS_NAME,
			STORAGE_EXT_PATH_MANAGER,
			STORAGE_EXT_IFACE_MANAGER,
			"GetDeviceInfoByID",
			g_variant_new("(i)", storage_id));
	if (!result) {
		_E("There is no storage with the storage id (%d)", storage_id); //LCOV_EXCL_LINE
		return -ENODEV;
	}

	g_variant_get(result, "(issssssisibii)",
			&info->type, &info->devnode, &info->syspath,
			&info->fs_usage, &info->fs_type,
			&info->fs_version, &info->fs_uuid,
			&info->readonly, &info->mount_point,
			&info->state, &info->primary,
			&info->flags, &info->storage_id);

	g_variant_unref(result);

	return 0;
}
