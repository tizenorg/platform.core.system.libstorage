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
#include <sys/statvfs.h>
#include <vconf.h>

#include "common.h"
#include "list.h"
#include "log.h"

#define SDCARD_PATH "/opt/storage/sdcard"

#ifndef __USE_FILE_OFFSET64
int __WEAK__ storage_get_external_memory_size(struct statvfs *buf);
#else
int __WEAK__ storage_get_external_memory_size64(struct statvfs *buf);
#endif

static dd_list *cb_list[STORAGE_CALLBACK_MAX];

static int sdcard_get_state(void)
{
	int val, ret;

	ret = vconf_get_int(VCONFKEY_SYSMAN_MMC_STATUS, &val);
	if (ret < 0)
		return -EPERM;

	switch (val) {
	case VCONFKEY_SYSMAN_MMC_MOUNTED:
		return STORAGE_STATE_MOUNTED;
	case VCONFKEY_SYSMAN_MMC_INSERTED_NOT_MOUNTED:
		return STORAGE_STATE_UNMOUNTABLE;
	case VCONFKEY_SYSMAN_MMC_REMOVED:
	default:
		break;
	}

	return STORAGE_STATE_REMOVED;
}

static int sdcard_get_space(unsigned long long *total, unsigned long long *available)
{
	storage_state_e state;
	struct statvfs s;
	int ret, t, a;

	state = sdcard_get_state();
	if (state < STORAGE_STATE_MOUNTED) {
		t = 0;
		a = 0;
	} else {	/* if sdcard is mounted */
#ifndef __USE_FILE_OFFSET64
		ret = storage_get_external_memory_size(&s);
#else
		ret = storage_get_external_memory_size64(&s);
#endif
		if (ret < 0)
			return -EPERM;

		t = (unsigned long long)s.f_frsize*s.f_blocks;
		a = (unsigned long long)s.f_bsize*s.f_bavail;
	}

	if (total)
		*total = t;
	if (available)
		*available = a;

	return 0;
}

static const char *sdcard_get_root(void)
{
	return SDCARD_PATH;
}

static void sdcard_state_cb(keynode_t* key, void* data)
{
	struct storage_cb_info *cb_info;
	dd_list *elem;
	storage_state_e state;

	state = sdcard_get_state();

	DD_LIST_FOREACH(cb_list[STORAGE_CALLBACK_STATE], elem, cb_info)
		cb_info->state_cb(cb_info->id, state, cb_info->user_data);
}

static int register_request(enum storage_cb_type type)
{
	switch (type) {
	case STORAGE_CALLBACK_STATE:
		return vconf_notify_key_changed(VCONFKEY_SYSMAN_MMC_STATUS,
				sdcard_state_cb, NULL);
	default:
		break;
	}

	return -EINVAL;
}

static int release_request(enum storage_cb_type type)
{
	switch (type) {
	case STORAGE_CALLBACK_STATE:
		return vconf_ignore_key_changed(VCONFKEY_SYSMAN_MMC_STATUS,
				sdcard_state_cb);
	default:
		break;
	}

	return -EINVAL;
}

static int sdcard_register_cb(enum storage_cb_type type, struct storage_cb_info *info)
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
		ret = register_request(type);
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

static int sdcard_unregister_cb(enum storage_cb_type type, struct storage_cb_info *info)
{
	struct storage_cb_info *cb_info;
	dd_list *elem;
	int ret, n;

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
	if (n == 0) {
		ret = release_request(type);
		if (ret < 0)
			return -EPERM;
	}

	return 0;
}

const struct storage_ops sdcard = {
	.type = STORAGE_TYPE_EXTERNAL,
	.root = sdcard_get_root,
	.get_state = sdcard_get_state,
	.get_space = sdcard_get_space,
	.register_cb = sdcard_register_cb,
	.unregister_cb = sdcard_unregister_cb,
};

STORAGE_OPS_REGISTER(&sdcard)
