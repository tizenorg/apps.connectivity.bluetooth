/*
* ug-setting-bluetooth-efl
*
* Copyright 2012 Samsung Electronics Co., Ltd
*
* Contact: Hocheol Seo <hocheol.seo@samsung.com>
*           GirishAshok Joshi <girish.joshi@samsung.com>
*           DoHyun Pyun <dh79.pyun@samsung.com>
*
* Licensed under the Flora License, Version 1.1 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.tizenopensource.org/license
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#ifndef __BT_UTIL_H__
#define __BT_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>
#include <bundle.h>
#include <Elementary.h>

#include "bt-type-define.h"

/* Major Device Mask */
#define MISCELLANEOUS_MAJOR_DEVICE_MASK (0x000000)
#define COMPUTER_MAJOR_DEVICE_MASK (0x000100)
#define PHONE_MAJOR_DEVICE_MASK (0x000200)
#define LAN_MAJOR_DEVICE_MASK (0x000300)
#define AV_MAJOR_DEVICE_MASK (0x000400)
#define PERIPHERAL_MAJOR_DEVICE_MASK (0x000500)
#define IMAGING_MAJOR_DEVICE_MASK (0x000600)
#define WEARABLE_MAJOR_DEVICE_MASK (0x000700)
#define TOY_MAJOR_DEVICE_MASK (0x000800)
#define UNCLASSIFIED_MAJOR_DEVICE_MASK (0x001F00)
#define OBJECT_TRANSFER_MAJOR_SERVICE_MASK (0x100000)


void _bt_lock_display(void);

void _bt_unlock_display(void);


gboolean _bt_util_get_service_mask_from_uuid_list(char **uuids,
					      int no_of_service,
				      bt_service_class_t *service_mask_list);

gboolean _bt_util_update_class_of_device_by_service_list(bt_service_class_t service_list,
						 bt_major_class_t *major_class,
						 bt_minor_class_t *minor_class);

void _bt_util_set_value(const char *req, unsigned int *search_type,
			unsigned int *op_mode);

gboolean _bt_util_store_get_value(const char *key,
			      bt_store_type_t store_type,
			      unsigned int size, void *value);

void _bt_util_launch_no_event(void *data, void *obj, void *event);

void _bt_util_set_list_disabled(Evas_Object *genlist, Eina_Bool disable);

void _bt_util_set_phone_name(void);

int _bt_util_get_phone_name(char *phone_name, int size);

char * _bt_util_get_timeout_string(int timeout);

int _bt_util_get_timeout_value(int index);

int _bt_util_get_timeout_index(int timeout);

gboolean _bt_util_is_battery_low(void);

gboolean _bt_util_is_flight_mode(void);

void _bt_util_addr_type_to_addr_string(char *address,
					       unsigned char *addr);

void _bt_util_addr_type_to_addr_result_string(char *address,
					       unsigned char *addr);

void _bt_util_addr_type_to_addr_net_string(char *address,
					       unsigned char *addr);

void _bt_util_addr_string_to_addr_type(unsigned char *addr,
						  const char *address);

void _bt_util_convert_time_to_string(unsigned int remain_time,
					char *buf, int size);

void _bt_util_free_device_uuids(bt_dev_t *item);

void _bt_util_free_device_item(bt_dev_t *item);

#ifdef __cplusplus
}
#endif
#endif				/* __BT_UTIL_H__ */
