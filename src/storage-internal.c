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
#include <limits.h>
#include <sys/statvfs.h>

#include "common.h"
#include "log.h"

#define INTERNAL_MEMORY_PATH	"/opt/usr/media"

#ifndef __USE_FILE_OFFSET64
int __WEAK__ storage_get_internal_memory_size(struct statvfs *buf);
#else
int __WEAK__ storage_get_internal_memory_size64(struct statvfs *buf);
#endif

static int internal_get_state(void)
{
	return STORAGE_STATE_MOUNTED;
}

static int internal_get_space(unsigned long long *total, unsigned long long *available)
{
	struct statvfs s;
	int ret;

#ifndef __USE_FILE_OFFSET64
	ret = storage_get_internal_memory_size(&s);
#else
	ret = storage_get_internal_memory_size64(&s);
#endif
	if (ret < 0)
		return -EPERM;

	if (total)
		*total = (unsigned long long)s.f_frsize*s.f_blocks;
	if (available)
		*available = (unsigned long long)s.f_bsize*s.f_bavail;
	return 0;
}

static const char *internal_get_root(void)
{
	return INTERNAL_MEMORY_PATH;
}

const struct storage_ops internal = {
	.type = STORAGE_TYPE_INTERNAL,
	.root = internal_get_root,
	.get_state = internal_get_state,
	.get_space = internal_get_space,
};

STORAGE_OPS_REGISTER(&internal)
