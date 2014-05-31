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

#include <app.h>
#include <efl_assist.h>

#include "bt-main.h"
#include "bt-main-view.h"
#include "bt-handler.h"
#include "bt-util.h"
#include "bt-string.h"

static bool app_create(void *data)
{
	FN_START;

	bt_app_data_t *ad;
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
	FN_END;
	return TRUE;
}

static void app_terminate(void *data)
{
	DBG("");
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

static void app_service(service_h service, void *data)
{
	FN_START;

	int ret;
	char *value = NULL;
	bt_app_data_t *ad;
	ad = (bt_app_data_t *)data;
	if (ad == NULL)
		return;

	if (ad->main_genlist != NULL) {
		DBG("Window raise");
		elm_win_raise(ad->window);
		return;
	}

	service_clone(&ad->service, service);

	ret = service_get_extra_data(service, "launch-type", &value);
	retm_if (ret != SERVICE_ERROR_NONE, "service failed [%d]", ret);

	_bt_util_set_value(value, &ad->search_type, &ad->launch_mode);

	if (ad->launch_mode == BT_LAUNCH_CONNECT_HEADSET) {
		if (_bt_get_paired_device_count(ad) == 1) {
			DBG("Launch mode is Headset");
			bt_dev_t *dev = eina_list_nth(ad->paired_device, 0);
			if(dev != NULL) {
				/* Check whether the only paired device is Headset */
				if (dev->major_class == BT_MAJOR_DEV_CLS_AUDIO &&
					(dev->service_list & BT_SC_A2DP_SERVICE_MASK) > 0) {
					DBG("Paired Item is Headset");
					if ((dev->connected_mask & BT_STEREO_HEADSET_CONNECTED) == 0) {
						DBG("Paired device count is 1. Autoconnecting [%s]",
							dev->name);
						_bt_connect_device(ad, dev);
						DBG("dev->status : %d", dev->status);
						if (dev->status == BT_CONNECTING) {
							_bt_create_autoconnect_popup(dev);
							return;
						}
					}
				}
			}
			_bt_destroy_app(ad);
			return;
		}
	}

	if (_bt_initialize_view(ad) < 0) {
		ERR("_bt_initialize_view failed");
		_bt_destroy_app(ad);
	}

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
	event_callback.service = app_service;
	event_callback.low_memory = NULL;
	event_callback.low_battery = NULL;
	event_callback.device_orientation = NULL;
	event_callback.language_changed = app_lang_changed;
	event_callback.region_format_changed = app_regeion_changed;

	memset(&ad, 0x0, sizeof(bt_app_data_t));
	setenv("EVAS_NO_DRI_SWAPBUF", "1", 1);

	return app_efl_main(&argc, &argv, &event_callback, &ad);

}
