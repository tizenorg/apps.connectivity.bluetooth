/*
* bluetooth
*
* Copyright 2012 Samsung Electronics Co., Ltd
*
* Contact: Hocheol Seo <hocheol.seo@samsung.com>
*           Injun Yang <injun.yang@samsung.com>
*           Seungyoun Ju <sy39.ju@samsung.com>
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

#ifndef __BT_POPUP_H__
#define __BT_POPUP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt-main.h"

void _bt_create_disconnection_query_popup(bt_dev_t *ad);
void _bt_create_unpair_query_popup(bt_dev_t *ad);
void _bt_destroy_popup(bt_app_data_t *ad);

#ifdef __cplusplus
}
#endif

#endif
