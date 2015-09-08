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

#include <app.h>
#include <efl_assist.h>

#include "bt-main.h"
#include "bt-main-view.h"
#include "bt-handler.h"
#include "bt-util.h"
#include "bt-string.h"

#define COLOR_TABLE "/usr/apps/org.tizen.bluetooth/shared/res/tables/org.tizen.bluetooth_ChangeableColorTable.xml"
#define FONT_TABLE "/usr/apps/org.tizen.bluetooth/shared/res/tables/org.tizen.bluetooth_FontInfoTable.xml"

static bool app_create(void *data)
{
	FN_START;

	bt_app_data_t *ad;
	Ea_Theme_Color_Table *color_table = NULL;
	Ea_Theme_Font_Table *font_table = NULL;


	ad = (bt_app_data_t *)data;
	ad->window = _bt_create_win(PACKAGE);
	if (ad->window == NULL)
		return FALSE;

	bindtextdomain(BT_COMMON_PKG, LOCALEDIR);
	textdomain(BT_COMMON_PKG);

	/* Handle rotation */
	if (elm_win_wm_rotation_supported_get(ad->window)) {
		int rots[4] = {0, 90, 180, 270};
		elm_win_wm_rotation_available_rotations_set(ad->window, (const int *)(&rots), 4);
	}

	ad->conn = (void *)dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);

	_bt_init(ad);

	/* Set Changeable color table */
	ea_theme_changeable_ui_enabled_set(EINA_TRUE);
	color_table = ea_theme_color_table_new(COLOR_TABLE);
	if (color_table) {
		ea_theme_colors_set(color_table, EA_THEME_STYLE_DEFAULT);
		ad->color_table = color_table;
	} else
		ERR("ea_theme_color_table_new fail!");

	font_table = ea_theme_font_table_new(FONT_TABLE);
	if (font_table) {
		ea_theme_fonts_set(font_table);
		ad->font_table = font_table;
	} else
		ERR("ea_theme_color_table_new fail!");

	FN_END;
	return TRUE;
}

static void app_terminate(void *data)
{
	DBG("");
	ret_if(!data);

	bt_app_data_t *ad = (bt_app_data_t *)data;
	if (ad->color_table) {
		ea_theme_color_table_free(ad->color_table);
		ad->color_table = NULL;
	}
	if (ad->font_table) {
		ea_theme_font_table_free(ad->font_table);
		ad->font_table = NULL;
	}
	return;
}

static void app_pause(void *data)
{
	DBG("");
	return;
}

static void app_resume(void *data)
{
	DBG("");
	return;
}

static Eina_Bool key_release_event_cb(void *data, int type, void *event)
{
	bt_app_data_t *ad = (bt_app_data_t *)data;
	Evas_Event_Key_Down *ev = event;
	int val = -1;

	if (!ev) {
		ERR("Invalid event object");
		return ECORE_CALLBACK_RENEW;
	}

	if (!ev->keyname) {
		ERR("key_release_event_cb : Invalid event keyname object");
		return ECORE_CALLBACK_RENEW;
	} else {
		DBG("key_release_event_cb : %s Down", ev->keyname);
	}

	if (!strcmp(ev->keyname, KEY_POWER)) {
		_bt_util_get_lcd_status(&val);
		DBG("%d", val);
		if (val <= 2) {
			_bt_destroy_app((void *)ad);
		}
	}

	return ECORE_CALLBACK_RENEW;
}

static void app_service(app_control_h service, void *data)
{
	FN_START;

	int ret;
	char *value = NULL;
	bt_app_data_t *ad;
	ad = (bt_app_data_t *)data;
	if (ad == NULL)
		return;

	if (ad->window && ad->layout_main != NULL) {
		INFO("raise window");
		elm_win_raise(ad->window);
		return;
	}

	app_control_clone(&ad->service, service);

	ad->key_release_handler = ecore_event_handler_add(ECORE_EVENT_KEY_UP,
					key_release_event_cb, ad);
	ret = app_control_get_extra_data(service, "launch-type", &value);
	retm_if (ret != APP_CONTROL_ERROR_NONE, "service failed [%d]", ret);

	_bt_util_set_value(value, &ad->search_type, &ad->launch_mode);
	INFO("Launch-type: %s, launch_mode: %d", value, ad->launch_mode);

	if (ad->do_auto_connect == TRUE) {
		DBG("auto headset called");
		_bt_auto_headset_connect(ad);
	}

	if (value)
		free(value);

	FN_END;
	return;
}

static void app_lang_changed(void *data)
{
	FN_START;
	ret_if(!data);
	bt_app_data_t *ad = (bt_app_data_t *)data;
	if (ad->scan_btn) {
		if (ad->op_status == BT_SEARCHING)
			elm_object_text_set(ad->scan_btn, STR_STOP);
		else
			elm_object_text_set(ad->scan_btn, STR_SCAN);
	}

	if ((ad->searched_device == NULL ||
		eina_list_count(ad->searched_device) == 0) &&
		(ad->paired_device == NULL ||
		eina_list_count(ad->paired_device) == 0))
			_bt_show_no_devices(ad);
	else
		_bt_update_device_list(ad);

#ifndef TELEPHONY_DISABLED
	if (ad->profile_vd) {
		if (ad->profile_vd->title_item)
			_bt_update_genlist_item(ad->profile_vd->title_item);
		if (ad->profile_vd->unpair_item)
			_bt_update_genlist_item(ad->profile_vd->unpair_item);
		if (ad->profile_vd->call_item)
			_bt_update_genlist_item(ad->profile_vd->call_item);
		if (ad->profile_vd->media_item)
			_bt_update_genlist_item(ad->profile_vd->media_item);
	}
#endif
	FN_END;
	return;
}

static void app_regeion_changed(void *data)
{
	return;
}

/**
* @describe
*   The entry of the program
*
* @param    argc
* @param    argv
* @param    int
* @exception
*/
DLL_DEFAULT int main(int argc, char *argv[])
{
	bt_app_data_t ad;
	app_event_callback_s event_callback;

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_service;
	event_callback.low_memory = NULL;
	event_callback.low_battery = NULL;
	event_callback.device_orientation = NULL;
	event_callback.language_changed = app_lang_changed;
	event_callback.region_format_changed = app_regeion_changed;

	memset(&ad, 0x0, sizeof(bt_app_data_t));
	setenv("EVAS_NO_DRI_SWAPBUF", "1", 1);

	return app_efl_main(&argc, &argv, &event_callback, &ad);

}
