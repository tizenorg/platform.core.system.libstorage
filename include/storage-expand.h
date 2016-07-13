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


#ifndef __STORAGE_EXPAND_H__
#define __STORAGE_EXPAND_H__

#ifdef __cplusplus
extern "C" {
#endif


 /**
 * @addtogroup CAPI_SYSTEM_STORAGE_MODULE
 * @{
 */

#include <tizen.h>

/**
 * @brief Enumeration of error codes for Storage.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 */
typedef enum {
	STORAGE_ERROR_NONE              = TIZEN_ERROR_NONE,                /**< Successful */
	STORAGE_ERROR_INVALID_PARAMETER = TIZEN_ERROR_INVALID_PARAMETER,   /**< Invalid parameter */
	STORAGE_ERROR_OUT_OF_MEMORY     = TIZEN_ERROR_OUT_OF_MEMORY,       /**< Out of memory */
	STORAGE_ERROR_NOT_SUPPORTED     = TIZEN_ERROR_NO_SUCH_DEVICE,      /**< Storage not supported */
	STORAGE_ERROR_OPERATION_FAILED  = TIZEN_ERROR_SYSTEM_CLASS | 0x12, /**< Operation failed */
} storage_error_e;


/**
 * @brief Enumeration of the storage types.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 */
typedef enum {
	STORAGE_TYPE_INTERNAL, /**< Internal device storage (built-in storage in a device, non-removable) */
	STORAGE_TYPE_EXTERNAL, /**< External storage */
} storage_type_e;


/**
 * @brief Enumeration of the state of storage devices.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 */
typedef enum {
	STORAGE_STATE_UNMOUNTABLE = -2, /**< Storage is present but cannot be mounted. Typically it happens if the file system of the storage is corrupted */
	STORAGE_STATE_REMOVED = -1, /**< Storage is not present */
	STORAGE_STATE_MOUNTED = 0, /**< Storage is present and mounted with read/write access */
	STORAGE_STATE_MOUNTED_READ_ONLY = 1, /**< Storage is present and mounted with read only access */
} storage_state_e;

/**
 * @brief Called to get information once for each supported storage.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] storage_id The unique storage ID
 * @param[in] type The type of the storage
 * @param[in] state The current state of the storage
 * @param[in] path The absolute path to the root directory of the storage
 * @param[in] user_data The user data passed from the foreach function
 *
 * @return @c true to continue with the next iteration of the loop, \n
 *         otherwise @c false to break out of the loop
 *
 * @pre	storage_foreach_device_supported() will invoke this callback function.
 * @see	storage_foreach_device_supported()
 */
typedef bool (*storage_device_supported_cb)(int storage_id, storage_type_e type,
		storage_state_e state, const char *path, void *user_data);

/**
 * @brief Retrieves all storage in a device.
 * @details This function invokes the callback function once for each storage in a device. \n
 *          If storage_device_supported_cb() returns @c false, then the iteration will be finished.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] callback The iteration callback function
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #STORAGE_ERROR_NONE               Successful
 * @retval #STORAGE_ERROR_INVALID_PARAMETER  Invalid parameter
 *
 * @post This function invokes storage_device_supported_cb() repeatedly for each supported device.
 * @see storage_device_supported_cb()
 */
int storage_foreach_device_supported(storage_device_supported_cb callback, void *user_data);

/**
 * @brief Gets the absolute path to the root directory of the given storage.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @remarks Files saved on the internal/external storage are readable or writable by all applications.\n
 * When an application is uninstalled, the files written by that application are not removed from the internal/external storage.\n
 * If you want to access files or directories in internal storage, you must declare http://tizen.org/privilege/mediastorage.\n
 * If you want to access files or directories in external storage, you must declare http://tizen.org/privilege/externalstorage.\n
 * You must release @a path using free().
 *
 * @param[in] storage_id The storage device
 * @param[out] path The absolute path to the storage directory
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #STORAGE_ERROR_NONE               Successful
 * @retval #STORAGE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @retval #STORAGE_ERROR_OUT_OF_MEMORY      Out of memory
 * @retval #STORAGE_ERROR_NOT_SUPPORTED      Storage not supported
 *
 * @see storage_get_state()
 */
int storage_get_root_directory(int storage_id, char **path);

/**
 * @brief Enumeration of the storage directory types
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 */
typedef enum {
	STORAGE_DIRECTORY_IMAGES,           /**< Image directory */
	STORAGE_DIRECTORY_SOUNDS,           /**< Sounds directory */
	STORAGE_DIRECTORY_VIDEOS,           /**< Videos directory */
	STORAGE_DIRECTORY_CAMERA,           /**< Camera directory */
	STORAGE_DIRECTORY_DOWNLOADS,        /**< Downloads directory */
	STORAGE_DIRECTORY_MUSIC,            /**< Music directory */
	STORAGE_DIRECTORY_DOCUMENTS,        /**< Documents directory */
	STORAGE_DIRECTORY_OTHERS,           /**< Others directory */
	STORAGE_DIRECTORY_SYSTEM_RINGTONES, /**< System ringtones directory. Only available for internal storage. */
	STORAGE_DIRECTORY_MAX
} storage_directory_e;

/**
 * @brief Gets the absolute path to the each directory of the given storage.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @remarks Files saved on the internal/external storage are readable or writable by all applications.\n
 * When an application is uninstalled, the files written by that application are not removed from the internal/external storage.\n
 * The directory path may not exist, so you must make sure that it exists before using it.\n
 * If you want to access files or directories in internal storage except #STORAGE_DIRECTORY_SYSTEM_RINGTONES, you must declare http://tizen.org/privilege/mediastorage.\n
 * If you want to access files or directories in #STORAGE_DIRECTORY_SYSTEM_RINGTONES, you must declare %http://tizen.org/privilege/systemsettings.\n
 * If you want to access files or directories in external storage, you must declare http://tizen.org/privilege/externalstorage.\n
 * You must release @a path using free().
 *
 * @param[in] storage_id The storage device
 * @param[in] type The directory type
 * @param[out] path The absolute path to the directory type
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #STORAGE_ERROR_NONE               Successful
 * @retval #STORAGE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @retval #STORAGE_ERROR_OUT_OF_MEMORY      Out of memory
 * @retval #STORAGE_ERROR_NOT_SUPPORTED      Storage not supported
 *
 * @see storage_get_state()
 */
int storage_get_directory(int storage_id, storage_directory_e type, char **path);

/**
 * @brief Gets the type of the given storage.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] storage_id The storage device
 * @param[out] type The type of the storage
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #STORAGE_ERROR_NONE               Successful
 * @retval #STORAGE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @retval #STORAGE_ERROR_NOT_SUPPORTED      Storage not supported
 */
int storage_get_type(int storage_id, storage_type_e *type);

/**
 * @brief Gets the current state of the given storage.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] storage_id The storage device
 * @param[out] state The current state of the storage
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #STORAGE_ERROR_NONE               Successful
 * @retval #STORAGE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @retval #STORAGE_ERROR_NOT_SUPPORTED      Storage not supported
 *
 * @see storage_get_root_directory()
 * @see storage_get_total_space()
 * @see storage_get_available_space()
 */
int storage_get_state(int storage_id, storage_state_e *state);

/**
 * @brief Called when the state of storage changes
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] storage_id The unique storage ID
 * @param[in] state The current state of the storage
 * @param[in] user_data The user data passed from the foreach function
 *
 * @pre	storage_set_state_changed_cb() will invoke this callback function.
 * @see	storage_set_state_changed_cb()
 * @see	storage_unset_state_changed_cb()
 */
typedef void (*storage_state_changed_cb)(int storage_id, storage_state_e state, void *user_data);

/**
 * @brief Registers a callback function to be invoked when the state of the storage changes.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] storage_id The storage device
 * @param[in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #STORAGE_ERROR_NONE               Successful
 * @retval #STORAGE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @retval #STORAGE_ERROR_NOT_SUPPORTED      Storage not supported
 * @retval #STORAGE_ERROR_OPERATION_FAILED Operation failed
 *
 * @post storage_state_changed_cb() will be invoked if the state of the registered storage changes.
 * @see storage_state_changed_cb()
 * @see storage_unset_state_changed_cb()
 */
int storage_set_state_changed_cb(int storage_id, storage_state_changed_cb callback, void *user_data);

/**
 * @brief Unregisters the callback function.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] storage_id The storage device to monitor
 * @param[in] callback The callback function to register
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #STORAGE_ERROR_NONE               Successful
 * @retval #STORAGE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @retval #STORAGE_ERROR_NOT_SUPPORTED      Storage not supported
 * @retval #STORAGE_ERROR_OPERATION_FAILED   Operation failed
 *
 * @see storage_state_changed_cb()
 * @see storage_set_state_changed_cb()
 */
int storage_unset_state_changed_cb(int storage_id, storage_state_changed_cb callback);

/**
 * @brief Enumeration of storage device types
 * @since_tizen 3.0
 */
typedef enum {
	STORAGE_DEV_EXT_SDCARD = 1001,     /**< sdcard device (external storage) */
	STORAGE_DEV_EXT_USB_MASS_STORAGE,  /**< USB storage device (external storage) */
} storage_dev_e;

/**
 * @brief Called when the state of a storage type changes.
 *
 * @since_tizen 3.0
 *
 * @param[in] storage_id The unique storage ID
 * @param[in] type The type of the storage device
 * @param[in] state The state of the storage
 * @param[in] fstype The type of the file system
 * @param[in] fsuuid The uuid of the file system
 * @param[in] mountpath The mount path of the file system
 * @param[in] primary The primary partition
 * @param[in] flags The flags for the storage status
 * @param[in] user_data The user data
 *
 * @pre	storage_set_changed_cb() will invoke this callback function.
 * @see	storage_set_changed_cb()
 * @see	storage_unset_changed_cb()
 */
typedef void (*storage_changed_cb)(int storage_id,
		storage_dev_e dev, storage_state_e state,
		const char *fstype, const char *fsuuid, const char *mountpath,
		bool primary, int flags, void *user_data);

/**
 * @brief Registers a callback function to be invoked when the state of the specified storage device type changes.
 *
 * @since_tizen 3.0
 *
 * @param[in] type The type of the storage device
 * @param[in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #STORAGE_ERROR_NONE               Successful
 * @retval #STORAGE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @retval #STORAGE_ERROR_NOT_SUPPORTED      Storage not supported
 * @retval #STORAGE_ERROR_OPERATION_FAILED   Operation failed
 *
 * @post storage_changed_cb() will be invoked if the state of the registered storage type changes.
 * @see storage_changed_cb()
 * @see storage_unset_changed_cb()
 */
int storage_set_changed_cb(storage_type_e type, storage_changed_cb callback, void *user_data);

/**
 * @brief Unregisters the callback function for storage type state changes.
 *
 * @since_tizen 3.0
 *
 * @param[in] type The type of the the storage device
 * @param[in] callback The callback function to unregister
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #STORAGE_ERROR_NONE               Successful
 * @retval #STORAGE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @retval #STORAGE_ERROR_NOT_SUPPORTED      Storage not supported
 * @retval #STORAGE_ERROR_OPERATION_FAILED   Operation failed
 *
 * @see storage_changed_cb()
 * @see storage_set_changed_cb()
 */
int storage_unset_changed_cb(storage_type_e type, storage_changed_cb callback);

/**
 * @brief Gets the total space of the given storage in bytes.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] storage_id The storage device
 * @param[out] bytes The total space size of the storage (bytes)
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #STORAGE_ERROR_NONE               Successful
 * @retval #STORAGE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @retval #STORAGE_ERROR_NOT_SUPPORTED      Storage not supported
 * @retval #STORAGE_ERROR_OPERATION_FAILED   Operation failed
 *
 * @see storage_get_state()
 * @see storage_get_available_space()
 */
int storage_get_total_space(int storage_id, unsigned long long *bytes);

/**
 * @brief Gets the available space size of the given storage in bytes.
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] storage_id The storage device
 * @param[out] bytes The available space size of the storage (bytes)
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #STORAGE_ERROR_NONE               Successful
 * @retval #STORAGE_ERROR_INVALID_PARAMETER  Invalid parameter
 * @retval #STORAGE_ERROR_NOT_SUPPORTED      Storage not supported
 * @retval #STORAGE_ERROR_OPERATION_FAILED   Operation failed
 *
 * @see storage_get_state()
 * @see storage_get_total_space()
 */
int storage_get_available_space(int storage_id, unsigned long long *bytes);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
