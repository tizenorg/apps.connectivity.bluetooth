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


#ifndef __BT_HANDLER_H__
#define __BT_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt-main.h"

gboolean _bt_send_result(bt_app_data_t *ad, bool result);
gboolean _bt_init(void *data);
void _bt_deinit(bt_app_data_t *ad);
void _bt_auto_headset_connect(bt_app_data_t *ad);

#ifdef __cplusplus
}
#endif
#endif
