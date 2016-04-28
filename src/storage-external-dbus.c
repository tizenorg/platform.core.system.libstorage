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

#define DBUS_REPLY_TIMEOUT (-1)

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
		if (err) {
			_E("fail to get dbus connection : %s", err->message);
			g_clear_error(&err);
		} else
			_E("fail to get dbus connection");
		return NULL;
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
		_E("fail to get dbus connection");
		return NULL;
	}

	ret = g_dbus_connection_call_sync(conn,
			dest, path, iface, method,
			param, NULL, G_DBUS_CALL_FLAGS_NONE,
			-1, NULL, &err);
	if (!ret) {
		if (err) {
			_E("dbus method sync call failed(%s)", err->message);
			g_clear_error(&err);
		} else
			_E("g_dbus_connection_call_sync() failed");
		return NULL;
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
		_E("Failed to get storage_ext device info");
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
			_E("malloc() failed");
			ret = -ENOMEM;
			goto err_out;
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
		storage_ext_release_internal(&info);
	}

	g_variant_iter_free(iter);
	g_variant_unref(result);

	return g_list_length(*list);

err_out:
	storage_ext_release_list(list);
	return ret;
}
