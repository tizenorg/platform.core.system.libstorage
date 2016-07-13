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


#ifndef __STORAGE_H__
#define __STORAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        storage.h
 * @ingroup     FRAMEWORK/SYSTEM
 * @brief       This file contains the API for the status of devices.
 * @author      TIZEN
 * @date        2013-02-15
 * @version     0.1
 */

 /**
 * @addtogroup CAPI_SYSTEM_STORAGE_MODULE
 * @{
 */

#include <sys/statvfs.h>
#include "storage-expand.h"

/**
 * @fn int storage_get_internal_memory_size(struct statvfs *buf)
 * @brief This generic API is used to get the internal memory size.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[out] buf A pointer to a statvfs structure
 * @return @c 0 on success,
 *         otherwise a negative error value on failure
 * @see
 * @par Example:
 * @code
 *	...
 *  struct statvfs s;
 *	if (storage_get_internal_memory_size(&s) < 0)
 *	dlog_print(DLOG_DEBUG, LOG_TAG, "Fail to get internal memory size");
 *	else
 *		dlog_print(DLOG_DEBUG, LOG_TAG, "Total mem : %lf, Avail mem : %lf",
 *				(double)s.f_frsize*s.f_blocks, (double)s.f_bsize*s.f_bavail);
 *	...
 * @endcode
 */
#ifndef __USE_FILE_OFFSET64
extern int storage_get_internal_memory_size(struct statvfs *buf);
#else
# define storage_get_internal_memory_size storage_get_internal_memory_size64
#endif

#ifdef __USE_FILE_OFFSET64
extern int storage_get_internal_memory_size64(struct statvfs *buf);
#endif

/**
 * @fn int storage_get_external_memory_size(struct statvfs *buf)
 * @brief This generic API is used to get the external memory size.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[out] buf A pointer to a statvfs structure
 * @return @c 0 on success,
 *         otherwise a negative error value on failure
 * @see
 * @par Example:
 * @code
 *	...
 *  struct statvfs s;
 *	if (storage_get_external_memory_size(&s) < 0)
 *		dlog_print(DLOG_DEBUG, LOG_TAG, "Fail to get external memory size");
 *	else
 *		dlog_print(DLOG_DEBUG, LOG_TAG, "Total mem : %lf, Avail mem : %lf",
 *				(double)s.f_frsize*s.f_blocks, (double)s.f_bsize*s.f_bavail);
 *	...
 * @endcode
 */
#ifndef __USE_FILE_OFFSET64
extern int storage_get_external_memory_size(struct statvfs *buf);
#else
# ifdef __REDIRECT_NTH
extern int __REDIRECT_NTH(storage_get_external_memory_size,
				(struct statvfs *buf), storage_get_external_memory_size64)
	__nonnull((1));
# else
#  define storage_get_external_memory_size storage_get_external_memory_size64
# endif
#endif

#ifdef __USE_FILE_OFFSET64
extern int storage_get_external_memory_size64(struct statvfs *buf);
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
