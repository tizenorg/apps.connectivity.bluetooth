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


#include "bt-main.h"
#include "bt-main-view.h"
#include "bt-string.h"
#include "bt-handler.h"
#include "bt-popup.h"


static void __bt_draw_popup(bt_app_data_t *ad,
			const char *title, const char *msg, char *btn1_text,
			char *btn2_text, void (*func) (void *data,
			Evas_Object *obj, void *event_info), void *data)
{
	FN_START;
	Evas_Object *btn1;
	Evas_Object *btn2;
	Evas_Object *scroller;
	Evas_Object *label;
	char *txt;

	ad->popup = elm_popup_add(ad->window);
	evas_object_size_hint_weight_set(ad->popup, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);

	if (title != NULL)
		elm_object_part_text_set(ad->popup, "title,text", title);

	if (msg != NULL) {
		scroller = elm_scroller_add(ad->popup);
		elm_object_style_set(scroller, "effect");
		evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);
		elm_object_content_set(ad->popup, scroller);
		evas_object_show(scroller);

		label = elm_label_add(scroller);
		elm_object_style_set(label, "popup/default");
		elm_label_line_wrap_set(label, ELM_WRAP_MIXED);

		txt = elm_entry_utf8_to_markup(msg);
		elm_object_text_set(label, txt);
		free(txt);

		evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(label, EVAS_HINT_FILL,
				EVAS_HINT_FILL);
		elm_object_content_set(scroller, label);
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
		DBG("op_status : %d", ad->op_status);
		if (ad->op_status == BT_UNPAIRING) {
			_bt_destroy_popup(ad);
			return;
		}

		ad->op_status = BT_UNPAIRING;
		elm_object_item_disabled_set(dev->genlist_item, EINA_TRUE);
		if (bt_device_destroy_bond(dev->addr_str) != BT_ERROR_NONE) {
			DBG("Fail to unpair");
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
/*		dev = _bt_get_dev_info(ad->paired_device, ad->paired_item);*/
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
	snprintf(message, BT_POPUP_TEXT_LENGTH,
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

	snprintf(message, BT_POPUP_TEXT_LENGTH,
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

