/*
 * Copyright (c) 2000-2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/

 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BT_VIEW_H__
#define __BT_VIEW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt-main.h"

#ifndef PREFIX
#define PREFIX "/usr/apps/org.tizen.bluetooth"
#endif

#define BT_IMAGE_PATH PREFIX"/res/images"
#define IMAGE_UNPAIR_BUTTON BT_IMAGE_PATH"/tw_list_setting_holo_dark.png"

#define BT_MAX_EVENT_STR_LENGTH	 50
#define BT_AUTO_CONNECT_SYSPOPUP_TIMEOUT_FOR_MULTIPLE_POPUPS 200

void _bt_connect_device(bt_app_data_t *ad, bt_dev_t *dev);
void _bt_disconnect_device(bt_app_data_t *ad, bt_dev_t *dev);
void _bt_create_group_title_item(bt_app_data_t *ad, const char *group);
void _bt_remove_group_title_item(bt_app_data_t *ad, const char *group);
void _bt_remove_paired_device_item(bt_app_data_t *ad, bt_dev_t *dev);
void _bt_remove_searched_device_item(bt_app_data_t *ad, bt_dev_t *dev);
void _bt_remove_all_searched_devices_item(bt_app_data_t *ad);
Elm_Object_Item *_bt_add_paired_device_item(bt_app_data_t *ad, bt_dev_t *dev);
Elm_Object_Item *_bt_add_searched_device_item(bt_app_data_t *ad, bt_dev_t *dev);
Elm_Object_Item *_bt_add_paired_device_item_on_bond(bt_app_data_t *ad,
					bt_dev_t *dev);

void _bt_sort_paired_devices(bt_app_data_t *ad, bt_dev_t *dev,
					int connected);
bt_dev_t *_bt_create_paired_device_info(void *data);
bt_dev_t *_bt_create_searched_device_info(void *data);
gboolean _bt_is_matched_profile(unsigned int search_type,
					 unsigned int major_class,
					 unsigned int service_class);
bt_dev_t *_bt_get_dev_info(Eina_List *list,
				Elm_Object_Item *genlist_item);
bt_dev_t *_bt_get_dev_info_by_address(Eina_List *list, char *address);
int _bt_check_and_update_device(Eina_List *list, char *addr, char *name);

Evas_Object* _bt_create_win(const char *name);
void _bt_set_win_level(void *data);
void _bt_show_no_devices(bt_app_data_t *ad);
void _bt_hide_no_devices(bt_app_data_t *ad);
int _bt_initialize_view(bt_app_data_t *ad);
void _bt_get_paired_devices(bt_app_data_t *ad);
int _bt_get_paired_device_count(bt_app_data_t *ad);
void _bt_update_genlist_item(Elm_Object_Item *item);
void _bt_update_device_list(bt_app_data_t *ad);
int _bt_check_and_update_device(Eina_List *list, char *addr, char *name);
int _bt_check_paired_device_list(bt_app_data_t *ad);
Evas_Object *_bt_create_list_view(bt_app_data_t *ad);
void _bt_clean_app(bt_app_data_t *ad);
void _bt_destroy_app(bt_app_data_t *ad);

void _bt_create_autoconnect_popup(bt_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif
