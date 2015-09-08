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


#include "bt-main.h"
#include "bt-main-view.h"
#include "bt-string.h"
#include "bt-handler.h"
#include "bt-util.h"
#include "bt-type-define.h"
#include "bt-dbus-method.h"
#include "bt-popup.h"
#include "bt-profile-view.h"
#include <syspopup_caller.h>
#include <utilX.h>

#define BT_AUTO_CONNECT_SYSPOPUP_MAX_ATTEMPT 3
#define BT_CTXPOPUP_HEIGHT 128

static Eina_Bool __pop_cb(void *data, Elm_Object_Item *it)
{
	FN_START;
	_bt_destroy_app(data);
	FN_END;
	return EINA_FALSE;
}

Evas_Object* _bt_create_win(const char *name)
{
	FN_START;
	Evas_Object *eo;
	int w;
	int h;

	eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
	if (eo) {
		elm_win_title_set(eo, name);
		elm_win_borderless_set(eo, EINA_TRUE);
		ecore_x_window_size_get(ecore_x_window_root_first_get(),
					&w, &h);
		evas_object_resize(eo, w, h);
	}

	FN_END;
	return eo;
}

static Evas_Object* __create_bg(Evas_Object *parent)
{
	Evas_Object *bg;

	retv_if(parent == NULL, NULL);

	bg = elm_bg_add(parent);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(parent, bg);
	evas_object_show(bg);

	return bg;
}

void _bt_set_win_level(void *data)
{
	FN_START;
	bt_app_data_t *ad = NULL;
	Ecore_X_Window xwin;

	ad = (bt_app_data_t *)data;

	/* Get x-window */
	xwin = elm_win_xwindow_get(ad->window);

	/* Set Notification window */
	ecore_x_netwm_window_type_set(xwin, ECORE_X_WINDOW_TYPE_NOTIFICATION);
	utilx_set_system_notification_level(ecore_x_display_get(), xwin, UTILX_NOTIFICATION_LEVEL_HIGH);
	FN_END;
	return;
}

static void __bt_scan_btn_cb(void *data, Evas_Object *obj,
					void *event_info)
{
	FN_START;

	int ret;
	bt_app_data_t *ad = NULL;

	retm_if(data == NULL, "Invalid argument: bt_ug_data is NULL");

	ad = (bt_app_data_t *)data;

	if (ad->op_status == BT_SEARCHING) {
		ret = bt_adapter_stop_device_discovery();
		if (ret != BT_ERROR_NONE) {
			ERR("Discovery Stop failed");
			return;
		}
		if (!elm_object_disabled_get(ad->scan_btn))
			elm_object_disabled_set(ad->scan_btn, EINA_TRUE);
	} else {
		ret = bt_adapter_start_device_discovery();
		if (ret != BT_ERROR_NONE && ret != BT_ERROR_NOW_IN_PROGRESS){
			ERR("Discovery start failed");
			return;
		}

		if (!elm_object_disabled_get(ad->scan_btn)) {
			DBG("disable scan button!");
			elm_object_disabled_set(ad->scan_btn, EINA_TRUE);
		}
	}

	FN_END;
}

static Evas_Object * __bt_create_scan_btn(bt_app_data_t *ad)
{
	retvm_if(!ad, NULL, "Invalid parameter!");
	retvm_if(!ad->layout_btn, NULL, "Invalid parameter!");

	Evas_Object *btn;

	btn = elm_button_add(ad->layout_btn);
	retvm_if(!btn, NULL, "elm_button_add fail!");
	elm_object_text_set(btn, STR_SCAN);
	evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_smart_callback_add(btn, "clicked", __bt_scan_btn_cb, ad);
	evas_object_show(btn);
	elm_object_part_content_set(ad->layout_btn, "elm.icon", btn);

	return btn;
}

static Evas_Object* __create_nocontents_button_layout(Evas_Object* parent)
{
	Evas_Object *ly;

	retv_if(parent == NULL, NULL);

	ly = elm_layout_add(parent);
	elm_layout_theme_set(ly, "layout", "nocontents_button", "default");
	evas_object_size_hint_weight_set (ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ly, EVAS_HINT_FILL, EVAS_HINT_FILL);
	return ly;
}

static Evas_Object* __create_layout_main(Evas_Object* parent)
{
	Evas_Object *layout;

	retv_if(parent == NULL, NULL);

	layout = elm_layout_add(parent);
	retv_if(layout == NULL, NULL);

	elm_layout_theme_set(layout, "layout", "application", "default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_part_content_set(parent, "elm.swallow.content", layout);
	evas_object_show(layout);

	return layout;
}

static char *__bt_search_title_text_get(void *data, Evas_Object *obj,
					    const char *part)
{
	bt_app_data_t *ad = data;

	if (g_strcmp0(part, "elm.text") != 0) {
		ERR("It is not in elm.text part");
		return NULL;
	}

	if (ad->op_status == BT_SEARCHING) {
		if (ad->searched_device == NULL ||
			eina_list_count(ad->searched_device) == 0) {
			return g_strdup(STR_SCANNING);
		}
	}
	return g_strdup(STR_AVAILABLE_DEV);
}

static char *__bt_paired_title_text_get(void *data, Evas_Object *obj,
					    const char *part)
{
	retv_if(!part, NULL);
	INFO("part : %s", part);

	if (g_strcmp0(part, "elm.text") != 0) {
		ERR("It is not in elm.text part");
		return NULL;
	}
	return g_strdup(STR_PAIRED_DEV);
}

static Evas_Object *__bt_search_title_icon_get(void *data, Evas_Object *obj,
					    const char *part)
{
	bt_app_data_t *ad;
	Evas_Object *progressbar;

	retv_if(data == NULL, NULL);

	if (g_strcmp0(part, "elm.icon") != 0)
		return NULL;

	ad = (bt_app_data_t *)data;
	INFO("op_status : %d", ad->op_status);
	if (ad->op_status == BT_SEARCHING) {
		progressbar = elm_progressbar_add(obj);
		elm_object_style_set(progressbar, "process/groupindex");
		evas_object_size_hint_align_set(progressbar,
				EVAS_HINT_FILL, 0.5);
		evas_object_size_hint_weight_set(progressbar,
				EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_progressbar_pulse(progressbar, EINA_TRUE);
		return progressbar;
	}

	return NULL;
}

static void __bt_unpair_icon_sel_cb(void *data, Evas_Object *obj,
				     void *event_info)
{
	FN_START

	bt_dev_t *dev = NULL;
	bt_app_data_t *ad;

	if (data == NULL)
		return;

	dev = (bt_dev_t *)data;
	ad = dev->ad;

	if (ad == NULL)
		return;

	INFO("op_status : %d", ad->op_status);
	if (ad->op_status == BT_UNPAIRING) {
		DBG("Unpairing... Skip the click event");
		return;
	}

	INFO("Selected device : %s", dev->name);
	DBG("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", dev->bd_addr[0],
	       dev->bd_addr[1], dev->bd_addr[2], dev->bd_addr[3],
	       dev->bd_addr[4], dev->bd_addr[5]);

#ifdef TELEPHONY_DISABLED
	DBG("B2");
	_bt_create_unpair_query_popup(dev);
#else
	DBG("B2-3G");
	_bt_create_profile_view(dev);
#endif

	FN_END
}

static Evas_Object *__bt_dev_item_icon_get(void *data, Evas_Object *obj,
					    const char *part)
{
	FN_START;

	Evas_Object *btn = NULL;
	Evas_Object *icon = NULL;
	bt_dev_t *dev = NULL;

	retv_if(data == NULL, NULL);

	dev = (bt_dev_t *)data;

	if (g_strcmp0(part, "elm.icon") != 0)
		return NULL;

	btn = elm_button_add(obj);
	elm_object_style_set(btn, "naviframe/title_icon");
	icon = elm_image_add(btn);
	elm_image_file_set(icon, IMAGE_UNPAIR_BUTTON, NULL);
	evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
	elm_image_resizable_set(icon, EINA_FALSE, EINA_FALSE);

	if (dev->status == BT_CONNECTING || dev->status == BT_DISCONNECTING) {
		elm_object_disabled_set(btn, EINA_TRUE);
		ea_theme_object_color_set(icon, "AO015D");
	} else {
		elm_object_disabled_set(btn, EINA_FALSE);
		ea_theme_object_color_set(icon, "AO015");
	}
	elm_object_part_content_set(btn, "icon", icon);
	evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	evas_object_smart_callback_add(btn, "clicked",
					   (Evas_Smart_Cb)
					   __bt_unpair_icon_sel_cb,
					   (void *)dev);
	evas_object_propagate_events_set(btn, EINA_FALSE);


	FN_END;
	return btn;
}

static gboolean __bt_is_connectable_device(bt_dev_t *dev)
{
	FN_START;

	bt_device_info_s *device_info = NULL;
	retvm_if(dev == NULL, FALSE, "dev is NULL");

	if (dev->service_list == 0) {
		if (bt_adapter_get_bonded_device_info
		    ((const char *)dev->addr_str,
		     &device_info) != BT_ERROR_NONE) {
			if (device_info)
				bt_adapter_free_device_info(device_info);

			ERR("No service list. Unable to get bonded device info");
			return FALSE;
		}
		_bt_util_get_service_mask_from_uuid_list
		    (device_info->service_uuid, device_info->service_count,
		     &dev->service_list);

		bt_adapter_free_device_info(device_info);

		if (dev->service_list == 0) {
			ERR("No service list");
			return FALSE;
		}
	}

	if ((dev->service_list & BT_SC_HFP_SERVICE_MASK) ||
	    (dev->service_list & BT_SC_HSP_SERVICE_MASK) ||
	    (dev->service_list & BT_SC_A2DP_SERVICE_MASK) ||
	    (dev->service_list & BT_SC_HID_SERVICE_MASK) ||
	    (dev->service_list & BT_SC_NAP_SERVICE_MASK)) {
		/* Connectable device */
		return TRUE;
	}

	FN_END;
	return FALSE;
}

char* __bt_convert_rgba_to_hex(int r, int g, int b, int a)
{
	int hexcolor = 0;
	char* string = NULL;

	string = g_try_malloc0(sizeof(char )* 255);

	hexcolor = (r << 24) + (g << 16) + (b << 8) + a;
	sprintf(string, "%08x", hexcolor );

	return string;
}

static char *__bt_dev_item_text_get(void *data, Evas_Object *obj,
					  const char *part)
{
	char *buf = NULL;
	bt_dev_t *dev = NULL;
	char *str = STR_PAIRED;
	int r = 0, g = 0, b = 0, a = 0;

	retv_if(!data, NULL);

	dev = (bt_dev_t *)data;
	DBG("part : %s", part);

	if (!strcmp(part, "elm.text") || !strcmp(part, "elm.text.1")) {
		str = elm_entry_utf8_to_markup(dev->name);
		INFO("Label : %s", str);

		return str;
	} else if (!strcmp(part, "elm.text.2")) {
		bt_app_data_t *ad = (bt_app_data_t *)dev->ad;
		retvm_if(!ad, NULL, "ad is NULL");

		INFO("dev status : %d, is_connected : %d", dev->status, dev->is_connected);
		if (dev->status == BT_IDLE) {
			if (__bt_is_connectable_device(dev) == FALSE)
				str = STR_PAIRED;
			else {
				if (dev->is_connected > 0) {
					if (ad->launch_mode == BT_LAUNCH_CONNECT_HEADSET) {
						if (dev->connected_mask & BT_STEREO_HEADSET_CONNECTED)
							str = STR_CONNECTED;
					} else if (ad->launch_mode == BT_LAUNCH_CALL) {
						if (dev->connected_mask & BT_HEADSET_CONNECTED)
							str = STR_CONNECTED;
					} else
						str = STR_CONNECTED;
				} else
					str = STR_PAIRED;
			}
		} else if (dev->status == BT_DEV_PAIRING)
			str = STR_PAIRING;
		else if (dev->status == BT_CONNECTING || dev->status == BT_SERVICE_SEARCHING)
			str = STR_CONNECTING;
		else if (dev->status == BT_DISCONNECTING)
			str = STR_DISCONNECTING;

		INFO("Sub label : %s", str);

		ea_theme_color_get("AT013",&r, &g, &b, &a,
					NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL);
		buf = g_strdup_printf("<color=#%s>%s</color>",
			__bt_convert_rgba_to_hex(r, g, b, a)
			, str);
	}
	return buf;
}

static void __bt_set_genlist_itc(bt_app_data_t *ad)
{
	FN_START;

	/* Paired device group title */
	ad->paired_title_itc = elm_genlist_item_class_new();
	if (ad->paired_title_itc) {
		ad->paired_title_itc->item_style = "groupindex";
		ad->paired_title_itc->func.text_get = __bt_paired_title_text_get;
		ad->paired_title_itc->func.content_get = NULL;
		ad->paired_title_itc->func.state_get = NULL;
		ad->paired_title_itc->func.del = NULL;
	}

	/* Available device group title */
	ad->searched_group_itc = elm_genlist_item_class_new();
	if (ad->searched_group_itc) {
		ad->searched_group_itc->item_style = "groupindex";
		ad->searched_group_itc->func.text_get = __bt_search_title_text_get;
		ad->searched_group_itc->func.content_get = __bt_search_title_icon_get;
		ad->searched_group_itc->func.state_get = NULL;
		ad->searched_group_itc->func.del = NULL;
	}

	/* Set item class for paired device */
	ad->device_itc = elm_genlist_item_class_new();
	if (ad->device_itc) {
		ad->device_itc->item_style = "2text.1icon.divider";
		ad->device_itc->func.text_get = __bt_dev_item_text_get;
		ad->device_itc->func.content_get = __bt_dev_item_icon_get;
		ad->device_itc->func.state_get = NULL;
		ad->device_itc->func.del = NULL;
	}

	/* Searched device */
	ad->searched_itc = elm_genlist_item_class_new();
	if (ad->searched_itc) {
		ad->searched_itc->item_style = "1text";
		ad->searched_itc->func.text_get = __bt_dev_item_text_get;
		ad->searched_itc->func.content_get = NULL;
		ad->searched_itc->func.state_get = NULL;
		ad->searched_itc->func.del = NULL;
	}

	/* Searched device while pairing */
	ad->searched_pairing_itc = elm_genlist_item_class_new();
	if (ad->searched_pairing_itc) {
		ad->searched_pairing_itc->item_style = "2text";
		ad->searched_pairing_itc->func.text_get = __bt_dev_item_text_get;
		ad->searched_pairing_itc->func.content_get = NULL;
		ad->searched_pairing_itc->func.state_get = NULL;
		ad->searched_pairing_itc->func.del = NULL;
	}

	FN_END;
}

static void __bt_release_genlist_itc(bt_app_data_t *ad)
{
	ret_if(!ad);
	if (ad->paired_title_itc) {
		elm_genlist_item_class_free(ad->paired_title_itc);
		ad->paired_title_itc = NULL;
	}

	if (ad->searched_group_itc) {
		elm_genlist_item_class_free(ad->searched_group_itc);
		ad->searched_group_itc = NULL;
	}

	if (ad->device_itc) {
		elm_genlist_item_class_free(ad->device_itc);
		ad->device_itc = NULL;
	}

	if (ad->searched_itc) {
		elm_genlist_item_class_free(ad->searched_itc);
		ad->searched_itc = NULL;
	}

	if (ad->searched_pairing_itc) {
		elm_genlist_item_class_free(ad->searched_pairing_itc);
		ad->searched_pairing_itc = NULL;
	}
}

static Evas_Object *__bt_create_genlist(bt_app_data_t *ad)
{
	FN_START;
	retv_if(!ad, NULL);
	retv_if(!ad->layout_btn, NULL);
	Evas_Object *gl = NULL;

	gl = elm_genlist_add(ad->layout_btn);
	elm_genlist_mode_set(gl, ELM_LIST_COMPRESS);
	__bt_set_genlist_itc(ad);
	elm_object_part_content_set(ad->layout_btn, "elm.swallow.content", gl);

	return gl;
}

static void __bt_clear_genlist(bt_app_data_t *ad)
{
	FN_START;
	ret_if(!ad);
	ret_if(!ad->main_genlist);

	elm_genlist_clear(ad->main_genlist);


	evas_object_del(ad->main_genlist);

	__bt_release_genlist_itc(ad);

	ad->paired_title_item = NULL;
	ad->searched_title_item = NULL;
	ad->paired_item = NULL;
	ad->searched_item = NULL;
	ad->main_genlist = NULL;
	FN_END;
}

void _bt_show_no_devices(bt_app_data_t *ad)
{
	FN_START;
	retm_if(ad == NULL, "ad is NULL!");
	retm_if(ad->layout_btn == NULL, "ad->layout_btn is NULL!");

	__bt_clear_genlist(ad);

	elm_object_part_content_set(ad->layout_btn, "elm.swallow.content", NULL);
	elm_object_part_text_set(ad->layout_btn, "elm.text", STR_NO_DEV);
	FN_END;
}

void _bt_hide_no_devices(bt_app_data_t *ad)
{
	FN_START;
	retm_if(ad == NULL, "ad is NULL!");
	retm_if(ad->layout_btn == NULL, "ad->layout_btn is NULL!");
	ret_if(ad->paired_device || ad->searched_device);
	ret_if(elm_object_part_text_get(ad->layout_btn, "elm.text") == NULL);

	elm_object_part_text_set(ad->layout_btn, "elm.text", NULL);

	if(!_bt_create_list_view(ad))
		ERR("_bt_create_list_view fail!");
	FN_END;
}

int _bt_initialize_view(bt_app_data_t *ad)
{
	FN_START;
	Elm_Object_Item *navi_it;

	retvm_if(ad == NULL, -1,  "ad is NULL!");
	retvm_if(ad->window == NULL, -1,  "window is NULL!");

	if (ad->launch_mode == BT_LAUNCH_CALL)
		_bt_set_win_level(ad);

	if (ad->bg == NULL) {
		ad->bg = __create_bg(ad->window);
		retvm_if(ad->bg == NULL, -1,  "fail to create bg!");
	}
	if (ad->layout_main == NULL) {
		ad->layout_main = __create_layout_main(ad->bg);
		retvm_if(ad->layout_main == NULL, -1,  "fail to create layout_main!");
	}
	if (ad->navi == NULL) {
		ad->navi = elm_naviframe_add(ad->layout_main);
		retvm_if(ad->navi == NULL, -1,  "fail to create naviframe!");
		elm_object_part_content_set(ad->layout_main, "elm.swallow.content", ad->navi);
		ea_object_event_callback_add(ad->navi, EA_CALLBACK_BACK,
		ea_naviframe_back_cb, NULL);

		evas_object_show(ad->navi);
	}

	if (ad->layout_btn == NULL) {
		ad->layout_btn = __create_nocontents_button_layout(ad->layout_main);
		retvm_if(ad->layout_btn == NULL, -1,  "fail to create layout_btn!");
		navi_it = elm_naviframe_item_push(ad->navi, NULL, NULL, NULL, ad->layout_btn, NULL);
		ad->navi_item = navi_it;
		elm_naviframe_item_title_enabled_set(navi_it, EINA_FALSE, EINA_FALSE);
		elm_naviframe_item_pop_cb_set(navi_it, __pop_cb, ad);
	}

	if (ad->scan_btn == NULL) {
		ad->scan_btn = __bt_create_scan_btn(ad);
		retvm_if(ad->scan_btn == NULL, -1,  "fail to create scan_btn!");
	}

	if (ad->op_status == BT_ACTIVATING) {
		if (ad->scan_btn)
			elm_object_disabled_set(ad->scan_btn, EINA_TRUE);
	} else {
		ad->main_genlist = _bt_create_list_view(ad);
		elm_object_part_content_set(ad->layout_btn, "elm.swallow.content",
					ad->main_genlist);
		evas_object_data_set(ad->main_genlist, "appdata", ad);
	}

	evas_object_show(ad->window);

	FN_END;
	return 0;
}

static void __bt_free_device_info(bt_dev_t *dev)
{
	ret_if(!dev);

	int i;

	if (dev->uuids) {
		for (i = 0; i < dev->uuid_count ; i++) {
			if (dev->uuids[i]) {
				g_free(dev->uuids[i]);
				dev->uuids[i] = NULL;
			}
		}
		g_free(dev->uuids);
		dev->uuids = NULL;
	}

	free(dev);
	dev = NULL;
}

static void __bt_free_paired_device(bt_app_data_t *ad)
{
	FN_START;
	retm_if(!ad, "ad is NULL!");
	retm_if(!ad->paired_device, "paired_device is NULL!");

	Eina_List *l = NULL;
	Eina_List *l_next = NULL;
	bt_dev_t *dev = NULL;

	EINA_LIST_FOREACH_SAFE(ad->paired_device, l, l_next, dev) {
		__bt_free_device_info(dev);
	}

	eina_list_free(ad->paired_device);
	ad->paired_device = NULL;
	FN_END;
}

static bool __bt_cb_adapter_bonded_device(bt_device_info_s *device_info,
					  void *user_data)
{
	FN_START;
	bt_dev_t *dev = NULL;
	gboolean connected = FALSE;
	bt_app_data_t *ad = NULL;
	unsigned int service_class;

	ad = (bt_app_data_t *)user_data;
	retv_if(ad == NULL, false);

	dev = _bt_create_paired_device_info(device_info);
	retv_if (!dev, false);
	INFO("[%s] is_connected : %d", device_info->remote_name, device_info->is_connected);
	dev->ad = (void *)ad;

	service_class = dev->service_class;

	if (_bt_is_matched_profile(ad->search_type,
					dev->major_class,
					service_class) == TRUE) {

		if (ad->launch_mode == BT_LAUNCH_CONNECT_HEADSET &&
			!(service_class & BT_COD_SC_RENDERING)) {
			DBG("Play via BT. A2DP is not supported");
			free(dev);
			return true;
		}

		if (dev->service_list & BT_SC_HFP_SERVICE_MASK ||
		    dev->service_list & BT_SC_HSP_SERVICE_MASK ||
		    dev->service_list & BT_SC_A2DP_SERVICE_MASK) {
			connected = _bt_util_is_profile_connected(BT_HEADSET_CONNECTED,
							dev->bd_addr);
			dev->connected_mask |= connected ? BT_HEADSET_CONNECTED : 0x00;

			connected =
					_bt_util_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
							dev->bd_addr);
			dev->connected_mask |=
			    connected ? BT_STEREO_HEADSET_CONNECTED : 0x00;
			if (dev->connected_mask == 0x00)
			        dev->is_connected = 0;
			else
			        dev->is_connected = 1;
		}

		if (_bt_add_paired_device_item(ad, dev) != NULL) {
			ad->paired_device =
				eina_list_append(ad->paired_device, dev);
		}
	} else {
		ERR("Device class and search type do not match");
		free(dev);
	}

	FN_END;
	return true;
}

static bool __bt_cb_adapter_create_paired_device_list
			(bt_device_info_s *device_info, void *user_data)
{
	FN_START;
	bt_dev_t *dev = NULL;
	gboolean connected = FALSE;
	bt_app_data_t *ad = NULL;
	unsigned int service_class;


	ad = (bt_app_data_t *)user_data;
	retv_if(ad == NULL, false);

	dev = _bt_create_paired_device_info(device_info);
	retv_if (!dev, false);

	dev->ad = (void *)ad;

	service_class = dev->service_class;

	if (_bt_is_matched_profile(ad->search_type,
					dev->major_class,
					service_class) == TRUE) {

		if (ad->launch_mode == BT_LAUNCH_CONNECT_HEADSET &&
			!(service_class & BT_COD_SC_RENDERING)) {
			DBG("Play via BT.  A2DP is not supported");
			free(dev);
			return true;
		}

		if (dev->service_list & BT_SC_HFP_SERVICE_MASK ||
		    dev->service_list & BT_SC_HSP_SERVICE_MASK ||
		    dev->service_list & BT_SC_A2DP_SERVICE_MASK) {
			connected = _bt_util_is_profile_connected(BT_HEADSET_CONNECTED,
							dev->bd_addr);
			dev->connected_mask |= connected ? BT_HEADSET_CONNECTED : 0x00;

			connected =
					_bt_util_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
							dev->bd_addr);
			dev->connected_mask |=
			    connected ? BT_STEREO_HEADSET_CONNECTED : 0x00;
		}

		dev->status = BT_IDLE;
		dev->ad = (void *)ad;
		dev->is_bonded = TRUE;
		ad->paired_device =
			eina_list_append(ad->paired_device, dev);
		_bt_update_device_list(ad);
	} else {
		ERR("Device class and search type do not match");
		free(dev);
	}

	FN_END;
	return true;
}

static void __bt_draw_paired_devices_items(bt_app_data_t *ad)
{
	FN_START;

	ret_if(ad == NULL);
	__bt_free_paired_device(ad);
	if (bt_adapter_foreach_bonded_device(__bt_cb_adapter_bonded_device,
					     (void *)ad) != BT_ERROR_NONE) {
		ERR("bt_adapter_foreach_bonded_device() failed");
		return;
	}

	FN_END;
	return;
}

void _bt_get_paired_devices(bt_app_data_t *ad)
{
	FN_START;

	ret_if(ad == NULL);
	__bt_free_paired_device(ad);
	if (bt_adapter_foreach_bonded_device(
			__bt_cb_adapter_create_paired_device_list,
			(void *)ad) != BT_ERROR_NONE) {
		ERR("bt_adapter_foreach_bonded_device() failed");
	}

	FN_END;
	return;
}

static int __bt_service_request_cb(void *data)
{
	FN_START;

	bt_app_data_t *ad = NULL;

	retvm_if(data == NULL, BT_APP_FAIL,
		 "Invalid argument: bt_app_data_t is NULL");

	ad = (bt_app_data_t *)data;

	if (ad->request_timer) {
		ecore_timer_del(ad->request_timer);
		ad->request_timer = NULL;
	}

	/* Need to modify API: Address parameter */
	if (ad->waiting_service_response == TRUE) {
		bt_dev_t *dev = NULL;

		ad->waiting_service_response = FALSE;
		bt_device_cancel_service_search();

		dev =
		    _bt_get_dev_info(ad->paired_device, ad->paired_item);
		retvm_if(dev == NULL, -1, "dev is NULL");

		dev->status = BT_IDLE;
		_bt_update_genlist_item(ad->paired_item);

		_bt_connect_device(ad, dev);
	} else {
		ad->paired_item = NULL;
	}

	FN_END;
	return BT_APP_ERROR_NONE;
}

static void __bt_paired_item_sel_cb(void *data, Evas_Object *obj,
					 void *event_info)
{
	FN_START;
	retm_if(data == NULL, "Invalid argument: data is NULL");
	retm_if(event_info == NULL, "Invalid argument: event_info is NULL");

	bt_app_data_t *ad = NULL;
	bt_dev_t *dev = NULL;
	Elm_Object_Item *item = NULL;
	int ret;

	elm_genlist_item_selected_set((Elm_Object_Item *) event_info,
				      EINA_FALSE);

	ad = (bt_app_data_t *)data;
	item = (Elm_Object_Item *)event_info;

	ret_if(ad->waiting_service_response == TRUE);
	ret_if(ad->op_status == BT_PAIRING);

	dev = _bt_get_dev_info(ad->paired_device, item);
	retm_if(dev == NULL, "Invalid argument: device info is NULL");
	if(dev->is_longpressed) {
		dev->is_longpressed = FALSE;
		return;
	}
	retm_if(dev->status != BT_IDLE,
		"Connecting / Disconnecting is in progress");

	if (ad->op_status == BT_SEARCHING) {
		ret = bt_adapter_stop_device_discovery();
		if (ret != BT_ERROR_NONE) {
			ERR("Fail to stop discovery");
			return;
		}
	}

	if ((ad->waiting_service_response) && (dev->service_list == 0)) {
		ERR("No service");

		ad->paired_item = item;
	}

	ad->paired_item = item;

	if (dev->service_list == 0) {
		DBG("Need to get service list");

		if (bt_device_start_service_search
		    ((const char *)dev->addr_str) == BT_ERROR_NONE) {

			dev->status = BT_SERVICE_SEARCHING;
			ad->waiting_service_response = TRUE;
			ad->request_timer =
			    ecore_timer_add(BT_SEARCH_SERVICE_TIMEOUT,
					    (Ecore_Task_Cb)
					    __bt_service_request_cb,
					    ad);

			_bt_update_genlist_item(ad->paired_item);
			return;
		} else {
			ERR("service search error");
			return;
		}
	}


	if (ad->launch_mode == BT_LAUNCH_CONNECT_HEADSET) {
		if (dev->connected_mask & BT_STEREO_HEADSET_CONNECTED)
			_bt_create_disconnection_query_popup(dev);
		else
			_bt_connect_device(ad, dev);
	} else if (ad->launch_mode == BT_LAUNCH_CALL) {
		if (dev->connected_mask & BT_HEADSET_CONNECTED)
			_bt_create_disconnection_query_popup(dev);
		else
			_bt_connect_device(ad, dev);
	} else {
		if (dev->connected_mask == 0) {
			/* Not connected case */
			_bt_connect_device(ad, dev);
		} else {
			/* connected case */
			DBG("Disconnect ??");

			_bt_create_disconnection_query_popup(dev);
		}
	}

	FN_END;
}

static void __bt_searched_item_sel_cb(void *data, Evas_Object *obj,
					   void *event_info)
{
	FN_START;

	bt_app_data_t *ad = NULL;
	bt_dev_t *dev = NULL;
	Elm_Object_Item *item = NULL;
	int ret;

	elm_genlist_item_selected_set((Elm_Object_Item *) event_info,
				      EINA_FALSE);

	retm_if(data == NULL, "Invalid argument: bt_app_data_t is NULL");

	ad = (bt_app_data_t *)data;

	ret_if(ad->op_status == BT_PAIRING);

	item = (Elm_Object_Item *) event_info;

	dev = _bt_get_dev_info(ad->searched_device,
				    (Elm_Object_Item *) event_info);
	retm_if(dev == NULL, "Invalid argument: device info is NULL");
	if(dev->is_longpressed) {
		dev->is_longpressed = FALSE;
		return;
	}

	ad->searched_item = item;

	if (_bt_util_is_battery_low() == TRUE) {
		/* Battery is critical low */
		// TODO : Make a warning popup
		DBG("Critical Low battery");
	}

	if (ad->op_status == BT_SEARCHING) {
		ret = bt_adapter_stop_device_discovery();
		if (ret != BT_ERROR_NONE)
			ERR("Fail to stop discovery");
	}

	if (ad->launch_mode == BT_LAUNCH_PICK) {
		_bt_destroy_app(ad);
		return;
	}

	if (bt_device_create_bond(dev->addr_str) == BT_ERROR_NONE) {
		dev->status = BT_DEV_PAIRING;
		ad->op_status = BT_PAIRING;
		_bt_lock_display();
		elm_genlist_item_item_class_update(dev->genlist_item,
				ad->searched_pairing_itc);
		_bt_update_genlist_item(item);
		if (ad->scan_btn)
			elm_object_disabled_set(ad->scan_btn, EINA_TRUE);
	}
	else {
		ad->searched_item = NULL;
	}

	FN_END;
}

static bool __bt_is_profile_connected(bt_app_data_t *ad,
						bt_audio_profile_type_e profile)
{
	FN_START;
	retv_if(!ad, FALSE);
	retv_if(!ad->paired_device, FALSE);

	bt_dev_t *dev = NULL;
	Eina_List *l = NULL;

	EINA_LIST_FOREACH(ad->paired_device, l, dev) {
		if (!dev)
			continue;
		if (profile == BT_AUDIO_PROFILE_TYPE_HSP_HFP) {
			if (dev->connected_mask & BT_HEADSET_CONNECTED) {
				return TRUE;
			}
		} else if (profile == BT_AUDIO_PROFILE_TYPE_A2DP) {
			if (dev->connected_mask & BT_STEREO_HEADSET_CONNECTED) {
				return TRUE;
			}
		}
	}

	return FALSE;

}

void _bt_connect_device(bt_app_data_t *ad, bt_dev_t *dev)
{
	FN_START;

	retm_if(ad == NULL, "ad is NULL");
	retm_if(dev == NULL, "dev is NULL");
	int headset_type = BT_AUDIO_PROFILE_TYPE_ALL;

	INFO("Request connection");

#ifdef TELEPHONY_DISABLED //B2
	DBG("B2");
	if (dev->service_list & BT_SC_A2DP_SERVICE_MASK) {
		if (bt_audio_connect(dev->addr_str,
				     BT_AUDIO_PROFILE_TYPE_A2DP) ==
		    BT_ERROR_NONE) {
			ad->connect_req = TRUE;
			dev->status = BT_CONNECTING;
			ad->connect_req_item = dev;
			_bt_util_disable_genlist_items(ad, EINA_TRUE);
			_bt_lock_display();
		} else {
			ERR("Fail to connect Headset device");
		}
	}
#else	//B2 3G
	DBG("B2 3G");
	if ((dev->service_list & BT_SC_HFP_SERVICE_MASK) ||
	    (dev->service_list & BT_SC_HSP_SERVICE_MASK)) {
		/* Connect the  Headset */
		if (dev->service_list & BT_SC_A2DP_SERVICE_MASK)
		{
			if (ad->launch_mode == BT_LAUNCH_CONNECT_HEADSET) {
				if (__bt_is_profile_connected(ad, BT_AUDIO_PROFILE_TYPE_HSP_HFP)) {
					INFO("HFP is already connected, connect A2DP only!");
					headset_type = BT_AUDIO_PROFILE_TYPE_A2DP;
				} else {
					INFO("Connect ALL profiles!");
					headset_type = BT_AUDIO_PROFILE_TYPE_ALL;
				}
			} else if (ad->launch_mode == BT_LAUNCH_CALL) {
				if (__bt_is_profile_connected(ad, BT_AUDIO_PROFILE_TYPE_A2DP)) {
					INFO("A2DP is already connected, connect HFP only!");
					headset_type = BT_AUDIO_PROFILE_TYPE_HSP_HFP;
				} else {
					INFO("Connect ALL profiles!");
					headset_type = BT_AUDIO_PROFILE_TYPE_ALL;
				}
			} else {
				if (!(dev->connected_mask & BT_HEADSET_CONNECTED) &&
					!(dev->connected_mask & BT_STEREO_HEADSET_CONNECTED))
					headset_type = BT_AUDIO_PROFILE_TYPE_ALL;
				else if (dev->connected_mask & BT_HEADSET_CONNECTED)
					headset_type = BT_AUDIO_PROFILE_TYPE_A2DP;
				else
					headset_type = BT_AUDIO_PROFILE_TYPE_HSP_HFP;
			}
		} else
			headset_type = BT_AUDIO_PROFILE_TYPE_HSP_HFP;

		INFO("Connection type = %d", headset_type);
		INFO("HFP connection status: %d", dev->connected_mask & BT_HEADSET_CONNECTED ? 1 : 0);
		INFO("A2DP connection status: %d", dev->connected_mask & BT_STEREO_HEADSET_CONNECTED ? 1 : 0);

		if (bt_audio_connect(dev->addr_str,
				     headset_type) == BT_ERROR_NONE) {
			ad->connect_req = TRUE;
			dev->status = BT_CONNECTING;
			ad->connect_req_item = dev;
			_bt_util_disable_genlist_items(ad, EINA_TRUE);
			if (ad->scan_btn)
				elm_object_disabled_set(ad->scan_btn, EINA_TRUE);
			_bt_lock_display();
		} else {
			ERR("Fail to connect Headset device");
		}
	} else if (dev->service_list & BT_SC_A2DP_SERVICE_MASK) {
		if (bt_audio_connect(dev->addr_str,
				     BT_AUDIO_PROFILE_TYPE_A2DP) ==
		    BT_ERROR_NONE) {
			ad->connect_req = TRUE;
			dev->status = BT_CONNECTING;
			ad->connect_req_item = dev;
			_bt_util_disable_genlist_items(ad, EINA_TRUE);
			_bt_lock_display();
		} else {
			ERR("Fail to connect Headset device");
		}
	}
#endif

	if (dev->genlist_item)
		_bt_update_genlist_item((Elm_Object_Item *) dev->genlist_item);

	FN_END;
}

void _bt_disconnect_device(bt_app_data_t *ad, bt_dev_t *dev)
{
	FN_START;

	ret_if(ad == NULL);
	ret_if(dev == NULL);

#ifdef TELEPHONY_DISABLED //B2
	DBG("B2");
	if (dev->service_list & BT_SC_A2DP_SERVICE_MASK) {
		if (bt_audio_disconnect(dev->addr_str,
				     BT_AUDIO_PROFILE_TYPE_A2DP) ==
		    BT_ERROR_NONE) {
			ad->disconnect_req = true;
			dev->status = BT_DISCONNECTING;
			ad->connect_req_item = dev;
			_bt_util_disable_genlist_items(ad, EINA_TRUE);
			_bt_lock_display();
		} else {
			ERR("Fail to connect Headset device");
		}
	}
#else	//B2 3G
	DBG("B2 3G");
	if (_bt_util_is_profile_connected(BT_HEADSET_CONNECTED,
						dev->bd_addr) == TRUE) {
		DBG("Disconnecting AG service");
		if (bt_audio_disconnect(dev->addr_str,
					BT_AUDIO_PROFILE_TYPE_ALL) ==
					BT_ERROR_NONE) {
			ad->disconnect_req = true;
			dev->status = BT_DISCONNECTING;
			ad->connect_req_item = dev;
			_bt_util_disable_genlist_items(ad, EINA_TRUE);
		}
	} else if (_bt_util_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
						dev->bd_addr) == TRUE) {
		DBG("Disconnecting AV service");
		if (bt_audio_disconnect(dev->addr_str,
					BT_AUDIO_PROFILE_TYPE_A2DP) ==
					BT_ERROR_NONE) {
			ad->disconnect_req = true;
			dev->status = BT_DISCONNECTING;
			ad->connect_req_item = dev;
			_bt_util_disable_genlist_items(ad, EINA_TRUE);
		}
	} else {
		ERR("Fail to connect Headset device");
	}
#endif
	if (dev->genlist_item)
		_bt_update_genlist_item((Elm_Object_Item *) dev->genlist_item);

	FN_END;
}

void _bt_create_group_title_item(bt_app_data_t *ad, const char *group)
{
	FN_START;

	retm_if(ad == NULL, "Invalid argument: ad is NULL");

	Elm_Object_Item *git = NULL;

	 if (g_strcmp0(group, GROUP_SEARCH) == 0) {
		git = elm_genlist_item_append(ad->main_genlist,
				ad->searched_group_itc, ad, NULL,
				ELM_GENLIST_ITEM_NONE, NULL, NULL);
		ad->searched_title_item = git;
	} else if (g_strcmp0(group, GROUP_PAIR) == 0) {
		if (ad->searched_title_item == NULL) {
			git = elm_genlist_item_append(ad->main_genlist,
				ad->paired_title_itc, ad, NULL,
				ELM_GENLIST_ITEM_NONE, NULL, NULL);
		} else {
			git = elm_genlist_item_insert_before(ad->main_genlist,
				ad->paired_title_itc, ad, NULL,
				ad->searched_title_item,
				ELM_GENLIST_ITEM_NONE, NULL, NULL);
		}
		ad->paired_title_item = git;
	} else {
		ERR("Invalid group");
		return;
	}

	elm_genlist_item_select_mode_set(git,
			ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	FN_END;
}

void _bt_remove_group_title_item(bt_app_data_t *ad, const char *group)
{
	FN_START;

	retm_if(ad == NULL, "Invalid argument: ad is NULL");
	retm_if(group == NULL, "Invalid argument: group is NULL");

	 if (g_strcmp0(group, GROUP_SEARCH) == 0) {
	 	if (ad->searched_title_item) {
			elm_object_item_del(ad->searched_title_item);
			ad->searched_title_item = NULL;
	 	}
	} else if (g_strcmp0(group, GROUP_PAIR) == 0) {
	 	if (ad->paired_title_item) {
			elm_object_item_del(ad->paired_title_item);
			ad->paired_title_item = NULL;
	 	}
	} else {
		ERR("Invalid group");
		return;
	}

	FN_END;
}

void _bt_remove_paired_device_item(bt_app_data_t *ad, bt_dev_t *dev)
{
	FN_START;

	bt_dev_t *item = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;

	retm_if(ad == NULL, "Invalid argument: ad is NULL");
	retm_if(dev == NULL, "Invalid argument: dev is NULL");

	EINA_LIST_FOREACH_SAFE(ad->paired_device, l, l_next, item) {
		if (item && (item == dev)) {
			if (item->genlist_item)
				elm_object_item_del(item->genlist_item);
			ad->paired_device =
			    eina_list_remove_list(ad->paired_device, l);
			_bt_util_free_device_item(item);
		}
	}

	if (ad->paired_device == NULL ||
		eina_list_count(ad->paired_device) == 0) {
		if (ad->paired_title_item) {
			elm_object_item_del(ad->paired_title_item);
			ad->paired_title_item = NULL;
		}
	}

	FN_END;
	return;
}
void _bt_remove_searched_device_item(bt_app_data_t *ad, bt_dev_t *dev)
{
	FN_START;

	bt_dev_t *item = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;

	ret_if(ad == NULL);
	ret_if(dev == NULL);

	EINA_LIST_FOREACH_SAFE(ad->searched_device, l, l_next, item) {
		if (item && (item == dev)) {
			if (dev->genlist_item) {
				elm_object_item_del(dev->genlist_item);
				dev->genlist_item = NULL;
			}
			ad->searched_device =
			    eina_list_remove_list(ad->searched_device, l);
			_bt_util_free_device_item(item);
		}
	}

	if (ad->searched_device == NULL ||
	    eina_list_count(ad->searched_device) == 0) {
		_bt_remove_group_title_item(ad, GROUP_SEARCH);

	}

	FN_END;
	return;
}

void _bt_remove_all_searched_devices_item(bt_app_data_t *ad)
{
	FN_START;

	bt_dev_t *dev = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;
	Elm_Object_Item *item;
	Elm_Object_Item *next;

	ret_if(ad == NULL);

	if (ad->searched_title_item) {
		item = elm_genlist_item_next_get(ad->searched_title_item);

		while (item != NULL) {
			next = elm_genlist_item_next_get(item);
			elm_object_item_del(item);
			item = next;
		}
	}

	EINA_LIST_FOREACH_SAFE(ad->searched_device, l, l_next, dev) {
		ad->searched_device =
			eina_list_remove_list(ad->searched_device, l);
		_bt_util_free_device_item(dev);
	}

	FN_END;
	return;
}

void _bt_sort_paired_devices(bt_app_data_t *ad, bt_dev_t *dev,
		int connected)
{
	FN_START;

	bt_dev_t *item = NULL;
	Elm_Object_Item *git = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;

	retm_if(ad == NULL, "Invalid argument: ugd is NULL");
	retm_if(dev == NULL, "Invalid argument: dev is NULL");

	dev->ad = ad;
	EINA_LIST_FOREACH_SAFE(ad->paired_device, l, l_next, item) {
		if (item && (item == dev)) {
			if (connected) {
				elm_object_item_del(item->genlist_item);
				git = elm_genlist_item_insert_after(ad->main_genlist,
						ad->device_itc, dev, NULL,
						ad->paired_title_item,
						ELM_GENLIST_ITEM_NONE,
						__bt_paired_item_sel_cb, ad);
				dev->genlist_item = git;
				break;
			} else {
				if (ad->searched_title_item) {
					elm_object_item_del(item->genlist_item);
					git = elm_genlist_item_insert_before(ad->main_genlist,
							ad->device_itc, dev, NULL,
							ad->searched_title_item,
							ELM_GENLIST_ITEM_NONE,
							__bt_paired_item_sel_cb,
							ad);
					dev->genlist_item = git;
					break;
				} else {
					elm_object_item_del(item->genlist_item);
					git = elm_genlist_item_append(ad->main_genlist,
							ad->device_itc, dev, NULL,
							ELM_GENLIST_ITEM_NONE,
							__bt_paired_item_sel_cb,
							ad);
					dev->genlist_item = git;
					break;
				}
			}
		}
	}

	FN_END;
	return;
}
Elm_Object_Item *_bt_add_paired_device_item_on_bond(bt_app_data_t *ad,
					bt_dev_t *dev)
{
	FN_START;

	Elm_Object_Item *git = NULL;

	retvm_if(ad == NULL, NULL, "Invalid argument: ad is NULL");
	retvm_if(dev == NULL, NULL, "Invalid argument: dev is NULL");

	if (ad->paired_title_item == NULL)
		_bt_create_group_title_item(ad, GROUP_PAIR);

	/* Add the device item in the list */
	if (ad->paired_device == NULL) {
		git = elm_genlist_item_insert_after(ad->main_genlist,
					ad->device_itc, dev, NULL,
					ad->paired_title_item,
					ELM_GENLIST_ITEM_NONE,
					__bt_paired_item_sel_cb, ad);
	} else {
		bt_dev_t *item_dev = NULL;
		Elm_Object_Item *item = NULL;
		Elm_Object_Item *next = NULL;

		item = elm_genlist_item_next_get(ad->paired_title_item);

		while (item != NULL || item != ad->searched_title_item) {
			item_dev = _bt_get_dev_info(ad->paired_device, item);

			if (item_dev && item_dev->is_connected > 0) {
				next = elm_genlist_item_next_get(item);
				if (next == NULL || next == ad->searched_title_item) {
					git = elm_genlist_item_insert_after(ad->main_genlist,
							ad->device_itc, dev,
							NULL, item,
							ELM_GENLIST_ITEM_NONE,
							__bt_paired_item_sel_cb,
							ad);
					break;
				}
				item = next;
			} else {
				git = elm_genlist_item_insert_before(ad->main_genlist,
						ad->device_itc, dev,
						NULL, item,
						ELM_GENLIST_ITEM_NONE,
						__bt_paired_item_sel_cb, ad);
				break;
			}
		}
	}

	dev->genlist_item = git;
	dev->status = BT_IDLE;
	dev->ad = (void *)ad;
	dev->is_bonded = TRUE;

	FN_END;

	return git;
}

static void __bt_ctxpopup_cb(void *data, Evas_Object *obj, void *event_info)
{
	FN_START;
	const char *label = elm_object_item_text_get((Elm_Object_Item *) event_info);
	if(label) fprintf(stderr, "text(%s) is clicked\n", label);

	Evas_Object *icon = elm_object_item_content_get((Elm_Object_Item *) event_info);
	if(icon) fprintf(stderr, "icon is clicked\n");
	FN_END;
}

static Eina_Bool __bt_ctxpopup_timer_cb(void *data)
{
	FN_START;
	bt_app_data_t *ad = (bt_app_data_t *)data;
	retv_if(!ad || !ad->ctxpopup_timer || !ad->ctxpopup, EINA_FALSE);

	ad->ctxpopup_timer = NULL;
	elm_ctxpopup_dismiss(ad->ctxpopup);
	FN_END;
	return EINA_FALSE;
}

static void __bt_move_ctxpopup(Evas_Object *ctxpopup, Elm_Object_Item *item)
{
	FN_START;
	ret_if(!ctxpopup || !item);

	Evas_Coord x, y, w, h;
	Evas_Object *track = elm_object_item_track(item);

	evas_object_geometry_get(track, &x, &y, &w, &h);

	elm_object_item_untrack(item);
	DBG("x : %d, y : %d, w : %d, h : %d", x, y, w, h);
	evas_object_move(ctxpopup, (w / 2), (y - BT_CTXPOPUP_HEIGHT >= 0) ? (y - BT_CTXPOPUP_HEIGHT) : (y + h));

	FN_END;
}

static void __bt_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	FN_START;
	bt_app_data_t *ad = (bt_app_data_t *)data;
	ret_if(!ad);

	evas_object_smart_callback_del(ad->ctxpopup,"dismissed", __bt_dismissed_cb);
	evas_object_del(ad->ctxpopup);
	ad->ctxpopup = NULL;
	FN_END;
}

static void __bt_create_ctxpopup_help(void *data, Evas_Object *obj, void *event_info)
{
	FN_START;
	bt_app_data_t *ad = (bt_app_data_t *)data;
	Evas_Object *ctxpopup = NULL;
	Elm_Object_Item *item = NULL;
	const Elm_Genlist_Item_Class *itc = NULL;
	bt_dev_t *dev = NULL;

	ret_if(!ad);

	item = (Elm_Object_Item *)event_info;

	itc = elm_genlist_item_item_class_get(item);
	retm_if(itc == NULL, "Invalid argument: genlist is NULL");

	dev = elm_object_item_data_get(item);
	retm_if(dev == NULL, "Invalid argument: device info is NULL");
	dev->is_longpressed = TRUE;

	ret_if(g_strcmp0(itc->item_style, "2text.1icon.divider") &&
		g_strcmp0(itc->item_style, "1text"));

	if(ad->ctxpopup) {
		evas_object_del(ad->ctxpopup);
		ad->ctxpopup = NULL;
	}

	ctxpopup = elm_ctxpopup_add(ad->navi);
	elm_object_style_set(ctxpopup, "help");
	ea_object_event_callback_add(ctxpopup, EA_CALLBACK_BACK, ea_ctxpopup_back_cb, NULL);
	evas_object_smart_callback_add(ctxpopup, "dismissed", __bt_dismissed_cb, ad);

	elm_ctxpopup_item_append(ctxpopup, elm_object_item_part_text_get(
			item, !g_strcmp0(itc->item_style, "1text") ?
			"elm.text" : "elm.text.1"),
			NULL, __bt_ctxpopup_cb, NULL);
	elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_DOWN,
						ELM_CTXPOPUP_DIRECTION_DOWN,
						ELM_CTXPOPUP_DIRECTION_DOWN,
						ELM_CTXPOPUP_DIRECTION_DOWN);
	ad->ctxpopup = ctxpopup;

	if (ad->ctxpopup_timer) {
		ecore_timer_del(ad->ctxpopup_timer);
		ad->ctxpopup_timer = NULL;
	}
	ad->ctxpopup_timer = ecore_timer_add(2.0, __bt_ctxpopup_timer_cb, ad);

	__bt_move_ctxpopup(ctxpopup, item);
	evas_object_show(ctxpopup);
	FN_END;
}

Elm_Object_Item *_bt_add_paired_device_item(bt_app_data_t *ad,
					bt_dev_t *dev)
{
	FN_START;

	Elm_Object_Item *git = NULL;

	retvm_if(ad == NULL, NULL, "Invalid argument: ad is NULL");
	retvm_if(dev == NULL, NULL, "Invalid argument: dev is NULL");

	if (ad->paired_title_item == NULL)
		_bt_create_group_title_item(ad, GROUP_PAIR);

	DBG("dev->status %d", dev->status);

	/* Add the device item in the list */
	if (ad->paired_device == NULL) {
		git = elm_genlist_item_insert_after(ad->main_genlist,
						ad->device_itc, dev, NULL,
						ad->paired_title_item,
						ELM_GENLIST_ITEM_NONE,
						__bt_paired_item_sel_cb, ad);
	} else {
		if (dev->is_connected > 0) {
			git = elm_genlist_item_insert_after(ad->main_genlist,
						ad->device_itc, dev, NULL,
						ad->paired_title_item,
						ELM_GENLIST_ITEM_NONE,
						__bt_paired_item_sel_cb, ad);
		} else {
			Elm_Object_Item *item = NULL;
			Elm_Object_Item *next = NULL;

			item = elm_genlist_item_next_get(ad->paired_title_item);

			while (item != NULL || item != ad->searched_title_item) {
				next = elm_genlist_item_next_get(item);
				if (next == NULL || next == ad->searched_title_item) {
					git = elm_genlist_item_insert_after(ad->main_genlist,
							ad->device_itc, dev,
							NULL, item,
							ELM_GENLIST_ITEM_NONE,
							__bt_paired_item_sel_cb, ad);
					break;
				}
				item = next;
			}
		}
	}

	dev->genlist_item = git;
	dev->status = BT_IDLE;
	dev->ad = (void *)ad;
	dev->is_bonded = TRUE;

	FN_END;

	return git;
}

Elm_Object_Item *_bt_add_searched_device_item(bt_app_data_t *ad, bt_dev_t *dev)
{
	FN_START;

	Elm_Object_Item *git = NULL;

	retvm_if(ad == NULL, NULL, "Invalid argument: ad is NULL");
	retvm_if(dev == NULL, NULL, "Invalid argument: dev is NULL");

	if (ad->searched_title_item == NULL)
		_bt_create_group_title_item(ad, GROUP_SEARCH);

	retvm_if(ad->searched_title_item == NULL, NULL,
		 "Fail to add searched title genlist item");

	/* Searched device Item */
	if (ad->searched_device == NULL) {
		git =
		    elm_genlist_item_insert_after(ad->main_genlist,
						  ad->searched_itc, dev, NULL,
						  ad->searched_title_item,
						  ELM_GENLIST_ITEM_NONE,
						  __bt_searched_item_sel_cb,
						  ad);
	} else {
		bt_dev_t *item_dev = NULL;
		Elm_Object_Item *item = NULL;
		Elm_Object_Item *next = NULL;

		item = elm_genlist_item_next_get(ad->searched_title_item);

		/* check the RSSI value of searched device list add arrange its order */
		while (item != NULL) {
			item_dev =
			    _bt_get_dev_info(ad->searched_device, item);
			retv_if(item_dev == NULL, NULL);

			if (item_dev->rssi > dev->rssi) {
				next = elm_genlist_item_next_get(item);
				if (next == NULL) {
					git =
					    elm_genlist_item_insert_after
					    (ad->main_genlist,
					     ad->searched_itc, dev, NULL, item,
					     ELM_GENLIST_ITEM_NONE,
					     __bt_searched_item_sel_cb,
					     ad);
					break;
				}
				item = next;
			} else {
				git =
				    elm_genlist_item_insert_before
				    (ad->main_genlist, ad->searched_itc, dev,
				     NULL, item, ELM_GENLIST_ITEM_NONE,
				     __bt_searched_item_sel_cb, ad);
				break;
			}
		}
	}

	dev->genlist_item = git;
	dev->status = BT_IDLE;
	dev->ad = (void *)ad;
	dev->is_bonded = FALSE;

	_bt_update_genlist_item(ad->searched_title_item);
	FN_END;

	return git;
}

bt_dev_t *_bt_create_paired_device_info(void *data)
{
	int i;
	unsigned char bd_addr[BT_ADDRESS_LENGTH_MAX];
	bt_dev_t *dev = NULL;
	bt_device_info_s *dev_info = NULL;

	retv_if(data == NULL, NULL);

	dev_info = (bt_device_info_s *) data;

	if (strlen(dev_info->remote_name) == 0) {
		ERR("Invalid device name");
		return NULL;
	}

	dev = malloc(sizeof(bt_dev_t));
	retv_if(dev == NULL, NULL);

	memset(dev, 0, sizeof(bt_dev_t));
	g_strlcpy(dev->name, dev_info->remote_name,
		  BT_DEVICE_NAME_LENGTH_MAX + 1);

	dev->major_class = dev_info->bt_class.major_device_class;
	dev->minor_class = dev_info->bt_class.minor_device_class;
	dev->service_class = dev_info->bt_class.major_service_class_mask;

	if (dev_info->service_uuid != NULL && dev_info->service_count > 0) {
		dev->uuids = g_new0(char *, dev_info->service_count + 1);

		for (i = 0; i < dev_info->service_count; i++) {
			dev->uuids[i] = g_strdup(dev_info->service_uuid[i]);
		}

		dev->uuid_count = dev_info->service_count;
	}

	_bt_util_addr_string_to_addr_type(bd_addr, dev_info->remote_address);

	memcpy(dev->addr_str, dev_info->remote_address, BT_ADDRESS_STR_LEN);

	memcpy(dev->bd_addr, bd_addr, BT_ADDRESS_LENGTH_MAX);

	_bt_util_get_service_mask_from_uuid_list(dev_info->service_uuid,
						 dev_info->service_count,
						 &dev->service_list);

	INFO("device name [%s]", dev->name);
	DBG("device major class [%x]", dev->major_class);
	DBG("device minor class [%x]", dev->minor_class);
	DBG("device service class [%x]", dev->service_class);
	INFO("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", dev->bd_addr[0],
	       dev->bd_addr[1], dev->bd_addr[2], dev->bd_addr[3],
	       dev->bd_addr[4], dev->bd_addr[5]);

	if (dev->major_class == BT_MAJOR_DEV_CLS_MISC &&
		dev->service_list != BT_SC_NONE) {
		_bt_util_update_class_of_device_by_service_list(dev->service_list,
				 &dev->major_class, &dev->minor_class);
	}

	return dev;
}

bt_dev_t *_bt_create_searched_device_info(void *data)
{
	int i;
	unsigned char bd_addr[BT_ADDRESS_LENGTH_MAX];
	bt_dev_t *dev = NULL;
	bt_adapter_device_discovery_info_s *dev_info = NULL;

	retv_if(data == NULL, NULL);

	dev_info = (bt_adapter_device_discovery_info_s *) data;

	if (strlen(dev_info->remote_name) == 0)
		return NULL;

	dev = calloc(1, sizeof(bt_dev_t));
	retv_if(dev == NULL, NULL);

	strncpy(dev->name, dev_info->remote_name, BT_DEVICE_NAME_LENGTH_MAX);

	dev->major_class = dev_info->bt_class.major_device_class;
	dev->minor_class = dev_info->bt_class.minor_device_class;
	dev->service_class = dev_info->bt_class.major_service_class_mask;
	dev->rssi = dev_info->rssi;

	if (dev_info->service_uuid != NULL && dev_info->service_count > 0) {
		dev->uuids = g_new0(char *, dev_info->service_count + 1);

		for (i = 0; i < dev_info->service_count; i++) {
			dev->uuids[i] = g_strdup(dev_info->service_uuid[i]);
		}

		dev->uuid_count = dev_info->service_count;
	}

	_bt_util_addr_string_to_addr_type(bd_addr, dev_info->remote_address);

	memcpy(dev->addr_str, dev_info->remote_address, BT_ADDRESS_STR_LEN);

	memcpy(dev->bd_addr, bd_addr, BT_ADDRESS_LENGTH_MAX);

	INFO("device name [%s]", dev->name);
	DBG("device major class [%x]", dev->major_class);
	DBG("device minor class [%x]", dev->minor_class);
	DBG("device service class [%x]", dev->service_class);
	INFO("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", dev->bd_addr[0],
	       dev->bd_addr[1], dev->bd_addr[2], dev->bd_addr[3],
	       dev->bd_addr[4], dev->bd_addr[5]);

	return dev;
}

gboolean _bt_is_matched_profile(unsigned int search_type,
				     unsigned int major_class,
				     unsigned int service_class)
{
	bt_device_major_mask_t major_mask = BT_DEVICE_MAJOR_MASK_MISC;

	if (search_type == 0x000000)
		return TRUE;

	INFO("search_type: %x", search_type);
	INFO("service_class: %x", service_class);

	/* Check the major class */
	switch (major_class) {
	case BT_MAJOR_DEV_CLS_COMPUTER:
		major_mask = BT_DEVICE_MAJOR_MASK_COMPUTER;
		break;
	case BT_MAJOR_DEV_CLS_PHONE:
		major_mask = BT_DEVICE_MAJOR_MASK_PHONE;
		break;
	case BT_MAJOR_DEV_CLS_LAN_ACCESS_POINT:
		major_mask = BT_DEVICE_MAJOR_MASK_LAN_ACCESS_POINT;
		break;
	case BT_MAJOR_DEV_CLS_AUDIO:
		major_mask = BT_DEVICE_MAJOR_MASK_AUDIO;
		break;
	case BT_MAJOR_DEV_CLS_PERIPHERAL:
		major_mask = BT_DEVICE_MAJOR_MASK_PERIPHERAL;
		break;
	case BT_MAJOR_DEV_CLS_IMAGING:
		major_mask = BT_DEVICE_MAJOR_MASK_IMAGING;
		break;
	case BT_MAJOR_DEV_CLS_WEARABLE:
		major_mask = BT_DEVICE_MAJOR_MASK_WEARABLE;
		break;
	case BT_MAJOR_DEV_CLS_TOY:
		major_mask = BT_DEVICE_MAJOR_MASK_TOY;
		break;
	case BT_MAJOR_DEV_CLS_HEALTH:
		major_mask = BT_DEVICE_MAJOR_MASK_HEALTH;
		break;
	default:
		major_mask = BT_DEVICE_MAJOR_MASK_MISC;
		break;
	}

	INFO("major_mask: %x", major_mask);

	if (search_type & major_mask)
		return TRUE;

	/* PTS AVRCP(CRC/BV-02-I) fail issue. display AVRCP only device in UI */
	if (search_type & BT_DEVICE_MAJOR_MASK_AUDIO &&
			service_class == BT_MAJOR_SERVICE_CLASS_AUDIO)
		return TRUE;

	return FALSE;
}

bt_dev_t *_bt_get_dev_info(Eina_List *list,
				Elm_Object_Item *genlist_item)
{
	bt_dev_t *item = NULL;
	Eina_List *l = NULL;

	retvm_if(list == NULL, NULL, "Invalid argument: list is NULL");
	retvm_if(genlist_item == NULL, NULL, "Invalid argument: obj is NULL");

	EINA_LIST_FOREACH(list, l, item) {
		if (item) {
			if (item->genlist_item == genlist_item)
				return item;
		}
	}

	return NULL;
}

bt_dev_t *_bt_get_dev_info_by_address(Eina_List *list, char *address)
{
	bt_dev_t *item = NULL;
	Eina_List *l = NULL;

	retvm_if(list == NULL, NULL, "Invalid argument: list is NULL");
	retvm_if(address == NULL, NULL, "Invalid argument: addr is NULL");

	EINA_LIST_FOREACH(list, l, item) {
		if (item) {
			if (memcmp(item->addr_str, address, BT_ADDRESS_STR_LEN)
			    == 0)
				return item;
		}
	}

	return NULL;
}

int _bt_check_and_update_device(Eina_List *list, char *addr, char *name)
{
	bt_dev_t *item = NULL;
	Eina_List *l = NULL;

	retv_if(list == NULL, -1);
	retv_if(addr == NULL, -1);
	retv_if(name == NULL, -1);

	EINA_LIST_FOREACH(list, l, item) {
		if (!item)
			continue;

		if (memcmp(item->addr_str, addr, BT_ADDRESS_STR_LEN) != 0)
			continue;

		memset(item->name, 0x00,
				BT_DEVICE_NAME_LENGTH_MAX);
		g_strlcpy(item->name, name,
				BT_DEVICE_NAME_LENGTH_MAX);

		if (item->genlist_item)
			_bt_update_genlist_item((Elm_Object_Item *)
					item->genlist_item);
		return 0;
	}

	return -1;
}

void _bt_update_genlist_item(Elm_Object_Item *item)
{
	FN_START;
	ret_if(!item);
	elm_genlist_item_update(item);
	FN_END;
}

void _bt_update_device_list(bt_app_data_t *ad)
{
	FN_START;
	Eina_List *l = NULL;
	bt_dev_t *dev = NULL;

	ret_if(ad == NULL);

	EINA_LIST_FOREACH(ad->paired_device, l, dev) {
		if (dev)
			_bt_update_genlist_item((Elm_Object_Item *)
					dev->genlist_item);
	}

	if (ad->paired_title_item)
		_bt_update_genlist_item(ad->paired_title_item);
	if (ad->searched_title_item)
		_bt_update_genlist_item(ad->searched_title_item);
	FN_END;
}

Evas_Object *_bt_create_list_view(bt_app_data_t *ad)
{
	FN_START;
	retv_if(!ad, NULL);
	retv_if(!ad->layout_btn, NULL);

	int ret;

	__bt_clear_genlist(ad);

	ad->main_genlist = __bt_create_genlist(ad);

	__bt_draw_paired_devices_items(ad);

	if(ad->paired_device == NULL) {
		if (ad->searched_title_item == NULL)
			_bt_create_group_title_item(ad, GROUP_SEARCH);
		if (ad->op_status == BT_ACTIVATED) {
			ret = bt_adapter_start_device_discovery();
			if (ret == BT_ERROR_NONE || ret == BT_ERROR_NOW_IN_PROGRESS) {
				ad->op_status = BT_SEARCHING;
				_bt_lock_display();
				_bt_update_genlist_item(ad->searched_title_item);
				elm_object_text_set(ad->scan_btn, STR_STOP);
			}
		}
	}
	evas_object_smart_callback_add(ad->main_genlist, "longpressed",
			__bt_create_ctxpopup_help, ad);

	evas_object_show(ad->main_genlist);

	FN_END;
	return ad->main_genlist;
}

int _bt_get_paired_device_count(bt_app_data_t *ad)
{
	FN_START;
	retvm_if(!ad, 0, "ad is NULL!");
	_bt_get_paired_devices(ad);
	retvm_if(!ad->paired_device, 0, "paired_device is NULL!");
	int count = eina_list_count(ad->paired_device);
	INFO("paired device count : %d", count);
	return count;
}

static gboolean __bt_auto_connect_system_popup_timer_cb(gpointer user_data)
{
	retv_if(user_data == NULL, FALSE);
	int ret;
	static int retry_count = 0;
	bundle *b = (bundle *)user_data;

	++retry_count;

	ret = syspopup_launch("bt-syspopup", b);
	if (ret < 0) {
		ERR("Sorry! Can't launch popup, ret=%d, Re-try[%d] time..",
							ret, retry_count);
		if (retry_count >= BT_AUTO_CONNECT_SYSPOPUP_MAX_ATTEMPT) {
			ERR("Sorry!! Max retry %d reached", retry_count);
			bundle_free(b);
			retry_count = 0;
			return FALSE;
		}
	} else {
		DBG("Finally Popup launched");
		retry_count = 0;
		bundle_free(b);
	}

	return (ret < 0) ? TRUE : FALSE;
}

void _bt_create_autoconnect_popup(bt_dev_t *dev)
{
	FN_START;

	int ret;
	bundle *b;
	b = bundle_create();
	retm_if (!b, "Unable to create bundle");

	bundle_add(b, "event-type", "music-auto-connect-request");

	ret = syspopup_launch("bt-syspopup", b);
	if (0 > ret) {
		ERR("Popup launch failed...retry %d", ret);

		g_timeout_add(BT_AUTO_CONNECT_SYSPOPUP_TIMEOUT_FOR_MULTIPLE_POPUPS,
				  (GSourceFunc)__bt_auto_connect_system_popup_timer_cb, b);
	} else {
		bundle_free(b);
	}

	FN_END;
}

void _bt_clean_app(bt_app_data_t *ad)
{
	FN_START;
	if (ad == NULL)
		return;

	__bt_clear_genlist(ad);

	if (ad->timer) {
		ecore_timer_del(ad->timer);
		ad->timer = NULL;
	}


	if (ad->popup) {
		evas_object_del(ad->popup);
		ad->popup = NULL;
	}

	if (ad->key_release_handler) {
		ecore_event_handler_del(ad->key_release_handler);
		ad->key_release_handler = NULL;
	}
	if (ad->service) {
		app_control_destroy(ad->service);
		ad->service = NULL;
	}
	if (ad->window) {
		evas_object_del(ad->window);
		ad->window = NULL;
	}

	FN_END;
}

void _bt_destroy_app(bt_app_data_t *ad)
{
	FN_START;
	_bt_unlock_display();

	if (ad) {
		_bt_send_result(ad, ad->a2dp_connected);

		/* Unref the Dbus connections */
		if (ad->conn)
			dbus_g_connection_unref(ad->conn);

		_bt_clean_app(ad);
		_bt_deinit(ad);
	}
	elm_exit();
}
