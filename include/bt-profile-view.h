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


#ifndef __BT_PROFILE_VIEW_H__
#define __BT_PROFILE_VIEW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <Elementary.h>

#include "bt-type-define.h"

typedef struct _bt_profile_view_data bt_profile_view_data;

struct _bt_profile_view_data {
	Evas_Object *win_main;
	Evas_Object *navi_bar;
	Evas_Object *genlist;
	Elm_Object_Item *navi_it;
	Elm_Object_Item *name_item;
	Elm_Object_Item *unpair_item;
	Elm_Object_Item *title_item;
	Elm_Object_Item *call_item;
	Elm_Object_Item *media_item;

	Elm_Genlist_Item_Class *name_itc;
	Elm_Genlist_Item_Class *unpair_itc;
	Elm_Genlist_Item_Class *title_itc;
	Elm_Genlist_Item_Class *call_itc;
	Elm_Genlist_Item_Class *media_itc;
};

void _bt_create_profile_view(bt_dev_t *dev_info);

void _bt_delete_profile_view(void *data);

#ifdef __cplusplus
}
#endif
#endif /* __BT_PROFILE_VIEW_H__ */
