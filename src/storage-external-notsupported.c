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
#include "common.h"

int storage_ext_get_space(int storage_id,
		unsigned long long *total, unsigned long long *available)
{
	return -ENOTSUP;
}

int storage_ext_foreach_device_list(storage_device_supported_cb callback, void *user_data)
{
	return -ENOTSUP;
}

int storage_ext_register_cb(enum storage_cb_type type, struct storage_cb_info *info)
{
	return -ENOTSUP;
}

int storage_ext_unregister_cb(enum storage_cb_type type, struct storage_cb_info *info)
{
	return -ENOTSUP;
}

int storage_ext_get_root(int storage_id, char *path, size_t len)
{
	return -ENOTSUP;
}

int storage_ext_get_state(int storage_id, storage_state_e *state)
{
	return -ENOTSUP;
}

int storage_ext_get_primary_mmc_path(char *path, size_t len)
{
	return -ENOTSUP;
}
