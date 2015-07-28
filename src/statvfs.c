/*
 * storage
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
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


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include <mntent.h>

#include "log.h"
#include "common.h"

#define MEMORY_GIGABYTE_VALUE  1073741824
#define MEMORY_MEGABYTE_VALUE  1048576

#define MEMORY_STATUS_USR_PATH "/opt/usr"
#define EXTERNAL_MEMORY_PATH   "/opt/storage/sdcard"
#define STORAGE_CONF_FILE      "/etc/storage/libstorage.conf"

/* it's for 32bit file offset */
struct statvfs_32 {
	unsigned long int f_bsize;
	unsigned long int f_frsize;
	unsigned long int f_blocks;
	unsigned long int f_bfree;
	unsigned long int f_bavail;
	unsigned long int f_files;
	unsigned long int f_ffree;
	unsigned long int f_favail;
	unsigned long int f_fsid;
#ifdef _STATVFSBUF_F_UNUSED
	int __f_unused;
#endif
	unsigned long int f_flag;
	unsigned long int f_namemax;
	int __f_spare[6];
};

#define MAX_LINE    128
#define MAX_SECTION 64
#define WHITESPACE  " \t"
#define NEWLINE     "\n\r"
#define COMMENT     '#'

#define MATCH(a, b)    (!strncmp(a, b, strlen(a)))
#define SET_CONF(a, b) (a = (b > 0.0 ? b : a))

struct parse_result {
	char *section;
	char *name;
	char *value;
};

struct storage_config_info {
	double total_size;
	double check_size;
	double reserved_size;
};

static struct storage_config_info storage_info;

static inline char *trim_str(char *s)
{
	char *t;
	/* left trim */
	s += strspn(s, WHITESPACE);

	/* right trim */
	for (t = strchr(s, 0); t > s; t--)
		if (!strchr(WHITESPACE, t[-1]))
			break;
	*t = 0;
	return s;
}

static int config_parse(const char *file_name, int cb(struct parse_result *result,
    void *user_data), void *user_data)
{
	FILE *f = NULL;
	struct parse_result result;
	/* use stack for parsing */
	char line[MAX_LINE];
	char section[MAX_SECTION];
	char *start, *end, *name, *value;
	int lineno = 0, ret = 0;

	if (!file_name || !cb) {
		ret = -EINVAL;
		goto error;
	}

	/* open conf file */
	f = fopen(file_name, "r");
	if (!f) {
		_E("Failed to open file %s", file_name);
		ret = -EIO;
		goto error;
	}

	/* parsing line by line */
	while (fgets(line, MAX_LINE, f) != NULL) {
		lineno++;

		start = line;
		start[strcspn(start, NEWLINE)] = '\0';
		start = trim_str(start);

		if (*start == COMMENT) {
			continue;
		} else if (*start == '[') {
			/* parse section */
			end = strchr(start, ']');
			if (!end || *end != ']') {
				ret = -EBADMSG;
				goto error;
			}

			*end = '\0';
			strncpy(section, start + 1, sizeof(section));
			section[MAX_SECTION-1] = '\0';
		} else if (*start) {
			/* parse name & value */
			end = strchr(start, '=');
			if (!end || *end != '=') {
				ret = -EBADMSG;
				goto error;
			}
			*end = '\0';
			name = trim_str(start);
			value = trim_str(end + 1);
			end = strchr(value, COMMENT);
			if (end && *end == COMMENT) {
				*end = '\0';
				value = trim_str(value);
			}

			result.section = section;
			result.name = name;
			result.value = value;
			/* callback with parse result */
			ret = cb(&result, user_data);
			if (ret < 0) {
				ret = -EBADMSG;
				goto error;
			}
		}
	}
	_D("Success to load %s", file_name);
	fclose(f);
	return 0;

error:
	if (f)
		fclose(f);
	_E("Failed to read %s:%d!", file_name, lineno);
	return ret;
}

static int load_config(struct parse_result *result, void *user_data)
{
	static int check_size = -1;
	struct storage_config_info *info = (struct storage_config_info *)user_data;
	char *name;
	char *value;

	if (!info)
		return -EINVAL;

	if (!MATCH(result->section, "STORAGE"))
		return -EINVAL;

	name = result->name;
	value = result->value;

	if (info->check_size > 0 && check_size < 0)
		check_size = (storage_info.total_size < info->check_size) ? 1 : 0;
	if (MATCH(name, "CHECK_SIZE"))
		info->check_size = atoi(value);
	else if (check_size == 0 && MATCH(name, "RESERVE"))
		info->reserved_size = atoi(value);
	else if (check_size == 1 && MATCH(name, "RESERVE_LITE"))
		info->reserved_size = atoi(value);

	return 0;
}

static void storage_config_load(struct storage_config_info *info)
{
	int ret;

	ret = config_parse(STORAGE_CONF_FILE, load_config, info);
	if (ret < 0)
		_E("Failed to load %s, %d Use default value!", STORAGE_CONF_FILE, ret);
}

static int get_memory_size(const char *path, struct statvfs_32 *buf)
{
	struct statvfs s;
	int ret;

	assert(buf);

	ret = statvfs(path, &s);
	if (ret)
		return -errno;

	memset(buf, 0, sizeof(struct statvfs_32));

	buf->f_bsize  = s.f_bsize;
	buf->f_frsize = s.f_frsize;
	buf->f_blocks = (unsigned long)s.f_blocks;
	buf->f_bfree  = (unsigned long)s.f_bfree;
	buf->f_bavail = (unsigned long)s.f_bavail;
	buf->f_files  = (unsigned long)s.f_files;
	buf->f_ffree  = (unsigned long)s.f_ffree;
	buf->f_favail = (unsigned long)s.f_favail;
	buf->f_fsid = s.f_fsid;
	buf->f_flag = s.f_flag;
	buf->f_namemax = s.f_namemax;

	return 0;
}

API int storage_get_internal_memory_size(struct statvfs *buf)
{
	struct statvfs_32 temp;
	static unsigned long reserved = 0;
	int ret;

	if (!buf) {
		_E("input param error");
		return -EINVAL;
	}

	ret = get_memory_size(MEMORY_STATUS_USR_PATH, &temp);
	if (ret || temp.f_bsize == 0) {
		_E("fail to get memory size");
		return -errno;
	}

	if (reserved == 0) {
		storage_info.total_size = (double)temp.f_frsize * temp.f_blocks;
		storage_config_load(&storage_info);
		reserved = (unsigned long)storage_info.reserved_size;
		reserved = reserved/temp.f_bsize;
		_I("total %4.4lf check %4.4lf reserved %4.4lf",
			storage_info.total_size, storage_info.check_size, storage_info.reserved_size);
	}
	if (temp.f_bavail < reserved)
		temp.f_bavail = 0;
	else
		temp.f_bavail -= reserved;

	memcpy(buf, &temp, sizeof(temp));
	return 0;
}

API int storage_get_internal_memory_size64(struct statvfs *buf)
{
	static unsigned long reserved = 0;
	int ret;

	if (!buf) {
		_E("input param error");
		return -EINVAL;
	}

	ret = statvfs(MEMORY_STATUS_USR_PATH, buf);
	if (ret) {
		_E("fail to get memory size");
		return -errno;
	}

	if (reserved == 0) {
		storage_info.total_size = (double)(buf->f_frsize * buf->f_blocks);
		storage_config_load(&storage_info);
		reserved = (unsigned long)storage_info.reserved_size;
		reserved = reserved/buf->f_bsize;
		_I("total %4.4lf check %4.4lf reserved %4.4lf",
			storage_info.total_size, storage_info.check_size, storage_info.reserved_size);
	}
	if (buf->f_bavail < reserved)
		buf->f_bavail = 0;
	else
		buf->f_bavail -= reserved;
	return 0;
}

static int mount_check(const char *path)
{
	int ret = false;
	struct mntent *mnt;
	const char *table = "/etc/mtab";
	FILE *fp;

	fp = setmntent(table, "r");
	if (!fp)
		return ret;
	while ((mnt = getmntent(fp))) {
		if (!strcmp(mnt->mnt_dir, path)) {
			ret = true;
			break;
		}
	}
	endmntent(fp);
	return ret;
}

API int storage_get_external_memory_size(struct statvfs *buf)
{
	struct statvfs_32 temp;
	int ret;

	_D("storage_get_external_memory_size");
	if (!buf) {
		_E("input param error");
		return -EINVAL;
	}

	if (!mount_check(EXTERNAL_MEMORY_PATH)) {
		memset(buf, 0, sizeof(struct statvfs_32));
		return 0;
	}

	ret = get_memory_size(EXTERNAL_MEMORY_PATH, &temp);
	if (ret) {
		_E("fail to get memory size");
		return -errno;
	}

	memcpy(buf, &temp, sizeof(temp));
	return 0;
}

API int storage_get_external_memory_size64(struct statvfs *buf)
{
	int ret;

	_D("storage_get_external_memory_size64");
	if (!buf) {
		_E("input param error");
		return -EINVAL;
	}

	if (!mount_check(EXTERNAL_MEMORY_PATH)) {
		memset(buf, 0, sizeof(struct statvfs));
		return 0;
	}

	ret = statvfs(EXTERNAL_MEMORY_PATH, buf);
	if (ret) {
		_E("fail to get memory size");
		return -errno;
	}

	return 0;
}
