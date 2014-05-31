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

#ifndef __BT_MAIN_H__
#define __BT_MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>
#include <dlog.h>
#include <app_service.h>
#include <Elementary.h>
#include <appcore-efl.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <app.h>
#include <efl_assist.h>
#include <Ecore_X.h>
#include <bluetooth.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <utilX.h>

#include "bt-type-define.h"


#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "BLUETOOTH"
#define DBG(format, args...) SLOGD(format, ##args)
#define ERR(format, args...) SLOGE(format, ##args)
#define DBG_SECURE(fmt, args...) SECURE_SLOGD(fmt, ##args)

#define	FN_START DBG("[ENTER FUNC]");
#define	FN_END DBG("[EXIT FUNC]");

#define ret_if(expr) \
	do { \
		if (expr) { \
			return; \
		} \
	} while (0);

#define retv_if(expr, val) \
	do { \
		if (expr) { \
			return (val); \
		} \
	} while (0);

#define retm_if(expr, fmt, arg...) \
	do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			return; \
		} \
	} while (0);


#define retvm_if(expr, val, fmt, arg...) \
	do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			return (val); \
		} \
	} while (0);


#define DLL_DEFAULT __attribute__((visibility ("default")))

#ifndef PACKAGE
#define PACKAGE "bluetooth"
#endif

#define BT_COMMON_PKG "bt-connection-popup"
#define LOCALEDIR "/usr/apps/org.tizen.bt-connection-popup/res/locale/"
#define GROUP_PAIR "Pair"
#define GROUP_SEARCH "Search"

typedef struct {
	Evas_Object *window;
	Evas_Object *bg;
	Evas_Object *layout_main;
	Evas_Object *layout_btn;
	Evas_Object *navi;
	Evas_Object *main_genlist;

	Evas_Object *popup;
	Ecore_Timer *timer;
	Evas_Object *scan_btn;

	/* Dbus connection / proxy */
	void *conn;

	/* Request timer */
	Ecore_Timer *request_timer;

	service_h service;
	unsigned int launch_mode;

	/* Paired / Searched device list */
	Eina_List *paired_device;
	Eina_List *searched_device;

	/* Selected device's genlist items */
	Elm_Object_Item *paired_item;
	Elm_Object_Item *searched_item;

	/* Genlist Item class */
	Elm_Genlist_Item_Class *paired_title_itc;
	Elm_Genlist_Item_Class *searched_group_itc;
	Elm_Genlist_Item_Class *searched_itc;
	Elm_Genlist_Item_Class *searched_pairing_itc;
	Elm_Genlist_Item_Class *device_itc;

	/* Genlist Items */
	Elm_Object_Item *navi_item;
	Elm_Object_Item *searched_title_item;
	Elm_Object_Item *paired_title_item;

	/*************************
	*		   Status Variables
	************************ */
	bool waiting_service_response;
	bool connect_req;
	bool search_req;
	bool a2dp_connected;
	unsigned int op_status;
	unsigned int search_type;

} bt_app_data_t;

#ifdef __cplusplus
}
#endif

#endif
