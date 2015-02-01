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
#include "bt-dbus-method.h"
#include "bt-popup.h"
#include "bt-util.h"

static void __bt_draw_popup(bt_app_data_t *ad,
			const char *title, const char *msg, char *btn1_text,
			char *btn2_text, void (*func) (void *data,
			Evas_Object *obj, void *event_info), void *data)
{
	FN_START;
	Evas_Object *btn1;
	Evas_Object *btn2;
	char *txt;

	ad->popup = elm_popup_add(ad->window);
	evas_object_size_hint_weight_set(ad->popup, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);

	if (title != NULL)
		elm_object_part_text_set(ad->popup, "title,text", title);

	if (msg != NULL) {
		DBG("msg : %s", msg);
		txt = elm_entry_utf8_to_markup(msg);
		elm_object_text_set(ad->popup, txt);
		free(txt);
	}

	if ((btn1_text != NULL) && (btn2_text != NULL)) {
		btn1 = elm_button_add(ad->popup);
		elm_object_style_set(btn1, "popup");
		elm_object_text_set(btn1, btn1_text);
		elm_object_part_content_set(ad->popup, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", func, data);

		btn2 = elm_button_add(ad->popup);
		elm_object_style_set(btn2, "popup");
		elm_object_text_set(btn2, btn2_text);
		elm_object_part_content_set(ad->popup, "button2", btn2);
		evas_object_smart_callback_add(btn2, "clicked", func, data);
	} else if (btn1_text != NULL) {
		btn1 = elm_button_add(ad->popup);
		elm_object_style_set(btn1, "popup");
		elm_object_text_set(btn1, btn1_text);
		elm_object_part_content_set(ad->popup, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", func, data);
	}

	evas_object_show(ad->popup);
	evas_object_show(ad->window);
	elm_object_focus_set(ad->popup, EINA_TRUE);

	FN_END;
}

static void __bt_unpair_confirm_cb(void *data,
					 Evas_Object *obj, void *event_info)
{
	FN_START;
	if (obj == NULL || data == NULL)
		return;

	bt_app_data_t *ad = NULL;
	bt_dev_t *dev = (bt_dev_t *)data;

	ret_if(dev->ad == NULL);
	ad = dev->ad;
	const char *event = elm_object_text_get(obj);

	if (!g_strcmp0(event, STR_OK)) {
		DBG("+ OK");
		INFO("op_status : %d", ad->op_status);
		if (ad->op_status == BT_UNPAIRING) {
			_bt_destroy_popup(ad);
			return;
		}

		ad->op_status = BT_UNPAIRING;
		elm_object_item_disabled_set(dev->genlist_item, EINA_TRUE);

		INFO("Request unpair");
		if (bt_device_destroy_bond(dev->addr_str) != BT_ERROR_NONE) {
			ERR("Fail to unpair");
			ad->op_status = BT_ACTIVATED;
		}
	}
	_bt_destroy_popup(ad);
	FN_END;
}

static void __bt_disconnection_confirm_cb(void *data,
					 Evas_Object *obj, void *event_info)
{
	FN_START;
	if (obj == NULL || data == NULL)
		return;

	bt_app_data_t *ad = NULL;
	bt_dev_t *dev = (bt_dev_t *)data;

	ret_if(dev->ad == NULL);
	ad = dev->ad;
	const char *event = elm_object_text_get(obj);

	if (!g_strcmp0(event, STR_OK)) {
		DBG("+ OK");
		INFO("Request all disconnection");

		_bt_disconnect_device(ad, dev);
	} else {
		DBG("+ Cancel");
	}

	_bt_destroy_popup(ad);
	FN_END;
}

void _bt_create_disconnection_query_popup(bt_dev_t *dev)
{
	FN_START;
	bt_app_data_t *ad = NULL;
	char message[BT_POPUP_TEXT_LENGTH] = { 0 };

	ret_if(dev->ad == NULL);
	ad = dev->ad;

	_bt_destroy_popup(ad);
	snprintf(message, sizeof(message),
			 STR_DISCONNECT_DEV_Q, dev->name);

	__bt_draw_popup(ad, STR_TITLE_DISCONNECT_Q,
			message,
			STR_CANCEL, STR_OK,
			__bt_disconnection_confirm_cb, dev);

	FN_END;
}

void _bt_create_unpair_query_popup(bt_dev_t *dev)
{
	FN_START;
	bt_app_data_t *ad = NULL;
	char message[BT_POPUP_TEXT_LENGTH] = { 0 };

	ret_if(dev->ad == NULL);
	ad = dev->ad;

	_bt_destroy_popup(ad);

	snprintf(message, sizeof(message),
			 STR_UNPAIR_DEV_Q, dev->name);

	__bt_draw_popup(ad, STR_TITLE_UNPAIR_Q,
			message,
			STR_CANCEL, STR_OK,
			__bt_unpair_confirm_cb, dev);

	FN_END;
}

void _bt_destroy_popup(bt_app_data_t *ad)
{
	FN_START;
	ret_if(ad == NULL);

	if (ad->popup) {
		evas_object_del(ad->popup);
		ad->popup = NULL;
	}
	FN_END;
}

#ifndef TELEPHONY_DISABLED
static void __bt_profile_call_disconnect_confirm_cb(void *data,
					 Evas_Object *obj, void *event_info)
{
	FN_START;
	if (obj == NULL || data == NULL)
		return;

	bt_app_data_t *ad = NULL;
	bt_dev_t *dev = (bt_dev_t *)data;
	gboolean connected = FALSE;
	Evas_Object *check = NULL;

	ret_if(dev->ad == NULL);
	ad = dev->ad;
	const char *event = elm_object_text_get(obj);

	if (!g_strcmp0(event, STR_OK)) {
		DBG("+ OK");

		connected = _bt_util_is_profile_connected(BT_HEADSET_CONNECTED,
						dev->bd_addr);

		if (connected == FALSE) {
			DBG("Not connected");
			_bt_destroy_popup(ad);
			return;
		}

		INFO("Request headset disconnection");
		if (bt_audio_disconnect(dev->addr_str,
				BT_AUDIO_PROFILE_TYPE_HSP_HFP) != BT_ERROR_NONE) {
			ERR("Fail to disconnect Headset device");
			_bt_destroy_popup(ad);
			return;
		}

		ad->connect_req = TRUE;
		dev->status = BT_DISCONNECTING;
		ad->connect_req_item = dev;
		if (ad->profile_vd)
			_bt_util_set_list_disabled(ad->profile_vd, EINA_TRUE);
		_bt_util_disable_genlist_items(ad, EINA_TRUE);
	} else {
		check = elm_object_item_part_content_get(ad->profile_vd->call_item, "elm.icon");
		if (check)
			elm_check_state_set(check, EINA_TRUE);
	}
	_bt_destroy_popup(ad);
	FN_END;
}

static void __bt_profile_media_disconnect_confirm_cb(void *data,
					 Evas_Object *obj, void *event_info)
{
	FN_START;
	if (obj == NULL || data == NULL)
		return;

	bt_app_data_t *ad = NULL;
	bt_dev_t *dev = (bt_dev_t *)data;
	gboolean connected = FALSE;
	Evas_Object *check = NULL;

	ret_if(dev->ad == NULL);
	ad = dev->ad;
	const char *event = elm_object_text_get(obj);

	if (!g_strcmp0(event, STR_OK)) {
		DBG("+ OK");

		connected = _bt_util_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
						dev->bd_addr);

		if (connected == FALSE) {
			DBG("Not connected");
			_bt_destroy_popup(ad);
			return;
		}

		INFO("Request a2dp disconnection");
		if (bt_audio_disconnect(dev->addr_str,
				BT_AUDIO_PROFILE_TYPE_A2DP) != BT_ERROR_NONE) {
			ERR("Fail to disconnect Headset device");
			_bt_destroy_popup(ad);
			return;
		}

		ad->connect_req = TRUE;
		dev->status = BT_DISCONNECTING;
		ad->connect_req_item = dev;
		if (ad->profile_vd)
			_bt_util_set_list_disabled(ad->profile_vd, EINA_TRUE);
		_bt_util_disable_genlist_items(ad, EINA_TRUE);
	} else {
		check = elm_object_item_part_content_get(ad->profile_vd->media_item, "elm.icon");
		if (check)
			elm_check_state_set(check, EINA_TRUE);
	}
	_bt_destroy_popup(ad);
	FN_END;
}

void _bt_create_call_option_disconnection_popup(bt_dev_t *dev)
{
	FN_START;
	bt_app_data_t *ad = NULL;
	char message[BT_POPUP_TEXT_LENGTH] = { 0 };

	ret_if(dev->ad == NULL);
	ad = dev->ad;

	_bt_destroy_popup(ad);

	snprintf(message, sizeof(message), BT_STR_DISABLE_PROFILE_OPTION,
						BT_STR_CALL_AUDIO, dev->name);

	__bt_draw_popup(ad, STR_TITLE_DISABLE_PROFILE,
			message,
			STR_CANCEL, STR_OK,
			__bt_profile_call_disconnect_confirm_cb, dev);

	FN_END;
}

void _bt_create_media_option_disconnection_popup(bt_dev_t *dev)
{
	FN_START;
	bt_app_data_t *ad = NULL;
	char message[BT_POPUP_TEXT_LENGTH] = { 0 };

	ret_if(dev->ad == NULL);
	ad = dev->ad;

	_bt_destroy_popup(ad);

	snprintf(message, sizeof(message), BT_STR_DISABLE_PROFILE_OPTION,
						BT_STR_MEDIA_AUDIO, dev->name);

	__bt_draw_popup(ad, STR_TITLE_DISABLE_PROFILE,
			message,
			STR_CANCEL, STR_OK,
			__bt_profile_media_disconnect_confirm_cb, dev);

	FN_END;
}
#endif
