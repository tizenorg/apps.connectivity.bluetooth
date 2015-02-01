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


#include <glib.h>
#include <bluetooth.h>
#include <Elementary.h>
#include <efl_assist.h>

#include "bt-main.h"
#include "bt-main-view.h"
#include "bt-popup.h"
#include "bt-dbus-method.h"
#include "bt-type-define.h"
#include "bt-string.h"
#include "bt-profile-view.h"
#include "bt-util.h"

/**********************************************************************
*                                               Static Functions
***********************************************************************/

static char *__bt_profile_text_get(void *data, Evas_Object *obj, const char *part)
{
	FN_START;
	bt_dev_t *dev = NULL;
	char *str = NULL;

	retv_if(!part, NULL);

	if (data == NULL)
		return NULL;

	dev = (bt_dev_t *)data;

	if (!strcmp(part, "elm.text"))
		str = elm_entry_utf8_to_markup(dev->name);

	FN_END;
	return str;
}

static void __bt_profile_unpair_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;
	bt_dev_t *dev = NULL;
	bt_app_data_t *ugd;

	ret_if(NULL == data);

	dev = (bt_dev_t *)data;

	if (event_info)
		elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
			EINA_FALSE);

	ugd = dev->ad;
	if (!ugd)
		return;

	if (ugd->op_status == BT_UNPAIRING)
		return;

	_bt_create_unpair_query_popup(dev);

	FN_END;
	return;
}

static char *__bt_profile_unpair_label_get(void *data, Evas_Object *obj,
					    const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };

	if (!strcmp(part, "elm.text")) {
		g_strlcpy(buf, BT_STR_UNPAIR, BT_GLOBALIZATION_STR_LENGTH);
	} else {
		DBG("empty text for label. \n");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static char *__bt_profile_title_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };

	if (strcmp(part, "elm.text") == 0) {
		/*Label */
		g_strlcpy(buf, BT_STR_PROFILE_OPTIONS, BT_GLOBALIZATION_STR_LENGTH);
	} else {
		DBG("This part name is not exist in style");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static char *__bt_profile_call_option_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };
	bt_dev_t *dev_info = NULL;
	bt_app_data_t *ugd = NULL;

	retv_if(NULL == data, NULL);
	dev_info = (bt_dev_t *)data;

	ugd = (bt_app_data_t *)(dev_info->ad);
	retv_if(NULL == ugd, NULL);

	if (!strcmp(part, "elm.text") || !strcmp(part, "elm.text.1")) {
		g_strlcpy(buf, BT_STR_CALL_AUDIO,
			BT_GLOBALIZATION_STR_LENGTH);
	} else if (!strcmp(part, "elm.text.2")) {
		if (dev_info->call_checked)
			g_strlcpy(buf, STR_CONNECTED,
					BT_GLOBALIZATION_STR_LENGTH);
		else
			g_strlcpy(buf, STR_CONNECT,
					BT_GLOBALIZATION_STR_LENGTH);
	}

	DBG("part[%s] : %s", part, buf);
	FN_END;
	return strdup(buf);
}

static char *__bt_profile_media_option_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };
	bt_dev_t *dev_info = NULL;
	bt_app_data_t *ugd = NULL;

	retv_if(NULL == data, NULL);
	dev_info = (bt_dev_t *)data;

	ugd = (bt_app_data_t *)(dev_info->ad);
	retv_if(NULL == ugd, NULL);

	if (!strcmp(part, "elm.text") || !strcmp(part, "elm.text.1")) {
		g_strlcpy(buf, BT_STR_MEDIA_AUDIO,
			BT_GLOBALIZATION_STR_LENGTH);
	} else if (!strcmp(part, "elm.text.2")) {
		if (dev_info->media_checked)
			g_strlcpy(buf, STR_CONNECTED,
					BT_GLOBALIZATION_STR_LENGTH);
		else
			g_strlcpy(buf, STR_CONNECT,
					BT_GLOBALIZATION_STR_LENGTH);
	}

	DBG("part[%s] : %s", part, buf);
	FN_END;
	return strdup(buf);
}

static Evas_Object *__bt_profile_call_option_icon_get(void *data, Evas_Object *obj,
					  const char *part)
{
	FN_START;

	Evas_Object *check = NULL;
	bt_dev_t *dev = NULL;

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	DBG("part [%s]", part);

	if (!strcmp(part, "elm.icon")) {
		check = elm_check_add(obj);

		elm_object_style_set(check, "list");
		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);

		evas_object_size_hint_align_set(check, EVAS_HINT_FILL,
				EVAS_HINT_FILL);

		elm_object_disabled_set(check, EINA_FALSE);

		dev->call_checked = dev->connected_mask & BT_HEADSET_CONNECTED;
		elm_check_state_pointer_set(check,
				(Eina_Bool *)&dev->call_checked);
		evas_object_repeat_events_set(check, EINA_TRUE);

	}

	FN_END;

	return check;
}

static Evas_Object *__bt_profile_media_option_icon_get(void *data, Evas_Object *obj,
					  const char *part)
{
	FN_START;

	Evas_Object *check = NULL;
	bt_dev_t *dev = NULL;

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	DBG("part [%s]", part);

	if (!strcmp(part, "elm.icon")) {
		check = elm_check_add(obj);
		elm_object_style_set(check, "list");
		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);

		evas_object_size_hint_align_set(check, EVAS_HINT_FILL,
				EVAS_HINT_FILL);

		elm_object_disabled_set(check, EINA_FALSE);

		dev->media_checked = dev->connected_mask & BT_STEREO_HEADSET_CONNECTED;

		elm_check_state_pointer_set(check,
				(Eina_Bool *)&dev->media_checked);
		evas_object_repeat_events_set(check, EINA_TRUE);
	}

	FN_END;

	return check;
}

int __bt_profile_connect_option(bt_app_data_t *ugd, bt_dev_t *dev,
				bt_device_type type)
{
	FN_START;

	int audio_profile;

	retv_if(ugd == NULL, -1);
	retv_if(dev == NULL, -1);

	if (type == BT_HEADSET_DEVICE || type == BT_STEREO_HEADSET_DEVICE) {
		if (type == BT_STEREO_HEADSET_DEVICE)
			audio_profile = BT_AUDIO_PROFILE_TYPE_A2DP;
		else
			audio_profile = BT_AUDIO_PROFILE_TYPE_HSP_HFP;

		if (bt_audio_connect(dev->addr_str,audio_profile) != BT_ERROR_NONE) {
			DBG("Fail to connect Headset device");
			return -1;
		}
	} else {
		DBG("Unknown type");
		return -1;
	}

	ugd->connect_req = TRUE;
	dev->status = BT_CONNECTING;
	_bt_update_genlist_item((Elm_Object_Item *)dev->genlist_item);

	if (ugd->profile_vd)
		_bt_util_set_list_disabled(ugd->profile_vd, EINA_TRUE);

	FN_END;
	return 0;
}


static void __bt_profile_call_option_item_sel(void *data, Evas_Object *obj,
					void *event_info)
{
	FN_START;

	bt_dev_t *dev = NULL;
	bt_app_data_t *ugd = NULL;
	Elm_Object_Item *item = NULL;

	ret_if(event_info == NULL);

	item = (Elm_Object_Item *)event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->ad == NULL);

	ugd = dev->ad;

	if (ugd->op_status == BT_UNPAIRING)
		return;

	if (dev->connected_mask & BT_HEADSET_CONNECTED) {
		/* connected case */
		if (ugd->popup) {
			evas_object_del(ugd->popup);
			ugd->popup = NULL;
		}

		_bt_create_call_option_disconnection_popup(dev);
	} else {
		__bt_profile_connect_option((bt_app_data_t *)dev->ad,
						dev, BT_HEADSET_DEVICE);
	}

	FN_END;
}

static void __bt_profile_media_option_item_sel(void *data, Evas_Object *obj,
					void *event_info)
{
	FN_START;

	bt_dev_t *dev = NULL;
	bt_app_data_t *ugd = NULL;
	Elm_Object_Item *item = NULL;

	ret_if(event_info == NULL);

	item = (Elm_Object_Item *)event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->ad == NULL);

	ugd = dev->ad;
	if (ugd->op_status == BT_UNPAIRING)
		return;

	if (dev->connected_mask & BT_STEREO_HEADSET_CONNECTED) {
		/* connected case */
		if (ugd->popup) {
			evas_object_del(ugd->popup);
			ugd->popup = NULL;
		}

		_bt_create_media_option_disconnection_popup(dev);

	} else {
		__bt_profile_connect_option((bt_app_data_t *)dev->ad,
						dev, BT_STEREO_HEADSET_DEVICE);
	}

	FN_END;
}

/* Create genlist and append items */
static Evas_Object *__bt_profile_draw_genlist(bt_app_data_t *ugd, bt_dev_t *dev_info)
{
	FN_START;

	bt_profile_view_data *vd = NULL;
	Evas_Object *genlist = NULL;
	Elm_Object_Item *git = NULL;

	retv_if(ugd == NULL, NULL);
	retv_if(ugd->profile_vd == NULL, NULL);

	vd = ugd->profile_vd;

	/* Set item class for dialogue normal items */

	vd->name_itc = elm_genlist_item_class_new();
	retv_if (vd->name_itc == NULL, NULL);

	vd->name_itc->item_style = "groupindex";
	vd->name_itc->func.text_get = __bt_profile_text_get;
	vd->name_itc->func.content_get = NULL;
	vd->name_itc->func.state_get = NULL;
	vd->name_itc->func.del = NULL;

	vd->unpair_itc = elm_genlist_item_class_new();
	retv_if (vd->unpair_itc == NULL, NULL);

	vd->unpair_itc->item_style = "1text";
	vd->unpair_itc->func.text_get = __bt_profile_unpair_label_get;
	vd->unpair_itc->func.content_get = NULL;
	vd->unpair_itc->func.state_get = NULL;
	vd->unpair_itc->func.del = NULL;

	/* Create genlist */
	genlist = elm_genlist_add(ugd->navi);

	/* device name item */
	git = elm_genlist_item_append(genlist, vd->name_itc, dev_info, NULL,
				    ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(git,
					ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	vd->name_item = git;

	/* unpair item */
	git = elm_genlist_item_append(genlist, vd->unpair_itc, NULL, NULL,
				    ELM_GENLIST_ITEM_NONE,
				    __bt_profile_unpair_item_sel, dev_info);

	vd->unpair_item = git;

	/* If the device has no headset profile, exit this function */
	if (!(dev_info->service_list & BT_SC_HFP_SERVICE_MASK) &&
	     !(dev_info->service_list & BT_SC_HSP_SERVICE_MASK) &&
	      !(dev_info->service_list & BT_SC_A2DP_SERVICE_MASK)) {
		return genlist;
	}

	vd->title_itc = elm_genlist_item_class_new();
	retv_if (vd->title_itc == NULL, NULL);

	vd->title_itc->item_style = "groupindex";
	vd->title_itc->func.text_get = __bt_profile_title_label_get;
	vd->title_itc->func.content_get = NULL;
	vd->title_itc->func.state_get = NULL;
	vd->title_itc->func.del = NULL;

	vd->call_itc = elm_genlist_item_class_new();
	retv_if (vd->call_itc == NULL, NULL);

	vd->call_itc->item_style = "2text.1icon.1";
	vd->call_itc->func.text_get = __bt_profile_call_option_label_get;
	vd->call_itc->func.content_get = __bt_profile_call_option_icon_get;
	vd->call_itc->func.state_get = NULL;
	vd->call_itc->func.del = NULL;

	vd->media_itc = elm_genlist_item_class_new();
	retv_if (vd->media_itc == NULL, NULL);

	vd->media_itc->item_style = "2text.1icon.1";
	vd->media_itc->func.text_get = __bt_profile_media_option_label_get;
	vd->media_itc->func.content_get = __bt_profile_media_option_icon_get;
	vd->media_itc->func.state_get = NULL;
	vd->media_itc->func.del = NULL;

	/* Profile options title */
	git = elm_genlist_item_append(genlist, vd->title_itc, NULL, NULL,
				    ELM_GENLIST_ITEM_NONE,
				    NULL, NULL);

	elm_genlist_item_select_mode_set(git,
				ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	vd->title_item = git;

	DBG("service list = %x", dev_info->service_list);
	if ((dev_info->service_list & BT_SC_HFP_SERVICE_MASK) ||
			(dev_info->service_list & BT_SC_HSP_SERVICE_MASK)) {
		/* Call audio */
		git = elm_genlist_item_append(genlist, vd->call_itc,
					dev_info, NULL,
					ELM_GENLIST_ITEM_NONE,
					__bt_profile_call_option_item_sel,
					dev_info);
		vd->call_item = git;
	}

	if (dev_info->service_list & BT_SC_A2DP_SERVICE_MASK) {
		/* Media audio */
		git = elm_genlist_item_append(genlist, vd->media_itc,
					dev_info, NULL,
					ELM_GENLIST_ITEM_NONE,
					__bt_profile_media_option_item_sel,
					dev_info);

		vd->media_item = git;
	}

	FN_END;

	return genlist;
}

static void __bt_clean_profile_view(void *data)
{
	FN_START;

	bt_app_data_t *ugd = (bt_app_data_t *)data;
	bt_profile_view_data *vd = NULL;

	ret_if(ugd == NULL);
	ret_if(ugd->profile_vd == NULL);

	vd = ugd->profile_vd;

	if (ugd->popup != NULL){
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}

	if (vd->name_itc) {
		elm_genlist_item_class_free(vd->name_itc);
		vd->name_itc = NULL;
	}

	if (vd->unpair_itc) {
		elm_genlist_item_class_free(vd->unpair_itc);
		vd->unpair_itc = NULL;
	}

	if (vd->title_itc) {
		elm_genlist_item_class_free(vd->title_itc);
		vd->title_itc = NULL;
	}

	if (vd->call_itc) {
		elm_genlist_item_class_free(vd->call_itc);
		vd->call_itc = NULL;
	}

	if (vd->media_itc) {
		elm_genlist_item_class_free(vd->media_itc);
		vd->media_itc = NULL;
	}

	free(vd);
	ugd->profile_vd = NULL;
	FN_END;
}

static Eina_Bool __bt_profile_back_clicked_cb(void *data, Elm_Object_Item *it)
{
	FN_START;
	retv_if(!data, EINA_TRUE);

	_bt_delete_profile_view(data);
	FN_END;
	return EINA_TRUE;
}

/**********************************************************************
*                                              Common Functions
***********************************************************************/

void _bt_create_profile_view(bt_dev_t *dev_info)
{
	FN_START;

	bt_profile_view_data *vd;
	bt_app_data_t *ugd;
	Evas_Object *genlist;
	Elm_Object_Item *navi_it;

	ret_if(dev_info == NULL);
	ret_if(dev_info->ad == NULL);

	ugd = dev_info->ad;

	vd = calloc(1, sizeof(bt_profile_view_data));
	ret_if(vd == NULL);

	ugd->profile_vd = vd;
	vd->win_main = ugd->window;
	vd->navi_bar = ugd->navi;

	genlist = __bt_profile_draw_genlist(ugd, dev_info);
	vd->genlist = genlist;

	/* Set ugd as genlist object data. */
	/* We can get this data from genlist object anytime. */
	evas_object_data_set(genlist, "view_data", vd);

	navi_it = elm_naviframe_item_push(vd->navi_bar, NULL,
					NULL, NULL, genlist, NULL);

	elm_naviframe_item_title_enabled_set(navi_it, EINA_FALSE, EINA_FALSE);

	elm_naviframe_prev_btn_auto_pushed_set(vd->navi_bar, EINA_FALSE);

	elm_naviframe_item_pop_cb_set(navi_it, __bt_profile_back_clicked_cb,
								ugd);

	vd->navi_it = navi_it;

	FN_END;

	return;
}

void _bt_delete_profile_view(void *data)
{
	FN_START;

	bt_app_data_t *ugd;

	ret_if(data == NULL);

	ugd = (bt_app_data_t *)data;

	elm_naviframe_item_pop(ugd->navi);
	__bt_clean_profile_view(ugd);

	FN_END;
}
