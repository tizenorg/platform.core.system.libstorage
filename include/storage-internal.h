/*
 * storage
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


#ifndef __STORAGE_INTERNAL_H__
#define __STORAGE_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup CAPI_SYSTEM_STORAGE_MODULE
 * @{
 */

#include <tizen.h>
#include "storage.h"

#define STORAGE_ERROR_NO_DEVICE TIZEN_ERROR_NO_SUCH_DEVICE

/**
 * @brief Get the mount path for the primary partition of sdcard.
 *
 * @since_tizen 3.0
 *
 * @remarks You must release the path using free() after use
 *
 * @param[out] storage_id The storage id
 * @param[out] path The mount path of sdcard primary partition.
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #STORAGE_ERROR_NONE               Successful
 * @retval #STORAGE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @retval #STORAGE_ERROR_NO_DEVICE          No such device
 */
int storage_get_primary_sdcard(int *storage_id, char **path);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif /* __STORAGE_INTERNAL_H__ */
