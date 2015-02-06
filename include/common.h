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


#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "storage-expand.h"

#ifndef API
#define API __attribute__ ((visibility("default")))
#endif

#ifndef __WEAK__
#define __WEAK__ __attribute__ ((weak))
#endif

#ifndef __CONSTRUCTOR__
#define __CONSTRUCTOR__ __attribute__ ((constructor))
#endif

#ifndef __DESTRUCTOR__
#define __DESTRUCTOR__ __attribute__ ((destructor))
#endif

enum storage_cb_type {
	STORAGE_CALLBACK_STATE,
	STORAGE_CALLBACK_MAX,
};

struct storage_cb_info {
	int id;
	storage_state_changed_cb state_cb;
	void *user_data;
};

struct storage_ops {
	storage_type_e type;
	const char *(*root) (void);
	int (*get_state) (void);
	int (*get_space) (unsigned long long *total, unsigned long long *available);
	int (*register_cb) (enum storage_cb_type type, struct storage_cb_info *info);
	int (*unregister_cb) (enum storage_cb_type type, struct storage_cb_info *info);
};

#define STORAGE_OPS_REGISTER(st)	\
static void __CONSTRUCTOR__ module_init(void)	\
{	\
	add_device(st);	\
}	\
static void __DESTRUCTOR__ module_exit(void)	\
{	\
	remove_device(st);	\
}

void add_device(const struct storage_ops *st);
void remove_device(const struct storage_ops *st);

#ifdef __cplusplus
}
#endif
#endif
