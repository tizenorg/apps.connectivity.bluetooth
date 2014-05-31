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

#include <bluetooth.h>
#include <aul.h>

#include "bt-main.h"
#include "bt-main-view.h"
#include "bt-handler.h"
#include "bt-string.h"
#include "bt-util.h"
#include "bt-popup.h"

static void __bt_ipc_update_connected_status(bt_app_data_t *ad,
					int connected_type,
					bool connected, int result,
					bt_address_t *addr)
{
	FN_START;

	bt_dev_t *item = NULL;
	char addr_str[BT_ADDRESS_STR_LEN + 1] = { 0 };

	_bt_util_addr_type_to_addr_string(addr_str, addr->bd_addr);

	item = _bt_get_dev_info_by_address(ad->paired_device, addr_str);

	ad->connect_req = FALSE;

	if (item == NULL)
		item = _bt_get_dev_info(ad->paired_device, ad->paired_item);

	retm_if(item == NULL, "fail to get dev info!");

	item->status = BT_IDLE;

	if (connected == TRUE) {
		item->connected_mask |= (result == BT_APP_ERROR_NONE) ? \
			connected_type : 0x00;
	} else {
		item->connected_mask &= (result == BT_APP_ERROR_NONE) ? \
			~connected_type : 0xFF;
	}

	_bt_destroy_popup(ad);
	if (ad->main_genlist)
		_bt_update_genlist_item((Elm_Object_Item *)item->genlist_item);

	FN_END;
}

static void __bt_adapter_state_changed(int result,
			bt_adapter_state_e adapter_state, void *user_data)
{
	retm_if (!user_data, "Invalid param");
	bt_app_data_t *ad = (bt_app_data_t *)user_data;
	retm_if (result != BT_ERROR_NONE, "BT Adapter operation is failed : %d", result);

	DBG("BT Adapter is %s", adapter_state == BT_ADAPTER_ENABLED ?
			"enabled" : "disabled");
	if (adapter_state == BT_ADAPTER_ENABLED) {
		ad->op_status = BT_ACTIVATED;
		if (ad->scan_btn)
			elm_object_disabled_set(ad->scan_btn, EINA_FALSE);
		if(!_bt_create_list_view(ad))
			ERR("_bt_create_list_view fail!");
	}
}

static bool __bt_cb_match_discovery_type(unsigned int major_class,
						unsigned int service_class,
						unsigned int mask)
{
	bt_device_major_mask_t major_mask = BT_DEVICE_MAJOR_MASK_MISC;

	if (mask == 0x000000)
		return TRUE;

	DBG("mask:[%x] service_class:[%x]", mask, service_class);

	/* Check the service_class */
	if (service_class & mask)
		return TRUE;

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

	DBG("major_mask: %x", major_mask);

	if (mask & major_mask)
		return TRUE;

	return FALSE;
}

static void __bt_cb_new_device_found(bt_adapter_device_discovery_info_s *info,
				     void *data)
{
	FN_START;
	bt_app_data_t *ad = NULL;
	bt_dev_t *dev = NULL;

	ret_if(info == NULL);
	ret_if(data == NULL);

	ad = (bt_app_data_t *)data;

	/* Check the service_class */
	if (__bt_cb_match_discovery_type(
				info->bt_class.major_device_class,
				info->bt_class.major_service_class_mask,
				ad->search_type) == FALSE) {
		DBG("No matched device type");
		return;
	}

	/* mono headset should not be displayed when BT app is launching in BT_LAUNCH_CONNECT_HEADSET */
	ret_if (!(info->bt_class.major_service_class_mask & BT_COD_SC_RENDERING));

	if (info->is_bonded == TRUE) {
		DBG("Bonded device");
		if (_bt_check_and_update_device(ad->paired_device,
					info->remote_address,
					info->remote_name) >= 0)
			return;
	}

	if (_bt_check_and_update_device(ad->searched_device,
					info->remote_address,
					info->remote_name) >= 0) {
		_bt_update_device_list(ad);
	} else {
		dev = _bt_create_searched_device_info((void *)info);
		if (dev == NULL) {
			ERR("Create new device item failed \n");
			return;
		}

		if (_bt_is_matched_profile(ad->search_type,
						dev->major_class,
						dev->service_class) == TRUE) {
			if (_bt_add_searched_device_item(ad, dev) == NULL) {
				ERR("Fail to add the searched device");
				return;
			}

			ad->searched_device =
			    eina_list_append(ad->searched_device, dev);

		} else {
			ERR("Searched device does not match the profile");
			free(dev);
		}
	}
	FN_END;
}

static void __bt_cb_search_completed(int result, void *data)
{
	FN_START;

	bt_app_data_t *ad = (bt_app_data_t *)data;

	_bt_unlock_display();
	ret_if(data == NULL);

	if (ad->op_status == BT_SEARCHING)
		ad->op_status = BT_ACTIVATED;

	elm_object_text_set(ad->scan_btn, STR_SCAN);

	if (ad->searched_device == NULL ||
		eina_list_count(ad->searched_device) == 0) {
		_bt_remove_group_title_item(ad, GROUP_SEARCH);
		if (ad->paired_device == NULL) {
			_bt_show_no_devices(ad);
		}
	} else
		_bt_update_genlist_item(ad->searched_title_item);

	FN_END;
}

static void _bt_cb_discovery_state_changed(int result,
			bt_adapter_device_discovery_state_e discovery_state,
			bt_adapter_device_discovery_info_s *discovery_info,
			void *user_data)
{
	ret_if(user_data == NULL);

	if (discovery_state == BT_ADAPTER_DEVICE_DISCOVERY_FOUND)
		__bt_cb_new_device_found(discovery_info, user_data);
	else if (discovery_state == BT_ADAPTER_DEVICE_DISCOVERY_FINISHED)
		__bt_cb_search_completed(result, user_data);

}

void _bt_cb_bonding_created(int result, bt_device_info_s *dev_info,
				void *user_data)
{
	FN_START;

	bt_app_data_t *ad = NULL;
	bt_dev_t *dev = NULL;

	_bt_unlock_display();

	ret_if(dev_info == NULL);
	ret_if(user_data == NULL);

	ad = (bt_app_data_t *)user_data;

	if (ad->op_status == BT_PAIRING)
		ad->op_status = BT_ACTIVATED;

	_bt_hide_no_devices(ad);

	dev = _bt_get_dev_info_by_address(ad->searched_device,
					dev_info->remote_address);

	if (dev == NULL)
		dev = _bt_get_dev_info(ad->searched_device, ad->searched_item);

	if (result != BT_ERROR_NONE) {
		DBG("Failed to pair with device : Error Cause[%d]", result);
		elm_object_disabled_set(ad->scan_btn, EINA_FALSE);
		if (dev == NULL)
			return;

		dev->status = BT_IDLE;

		elm_genlist_item_item_class_update(dev->genlist_item,
						ad->searched_itc);
		_bt_update_genlist_item((Elm_Object_Item *)dev->genlist_item);

		if (result != BT_ERROR_CANCELLED) {
			if (_bt_util_is_battery_low() == TRUE) {
				// TODO : make a warning popup
			}
		}
	} else {
		bt_dev_t *new_dev = NULL;
		Elm_Object_Item *item = NULL;

		if (_bt_check_and_update_device(ad->paired_device,
					dev_info->remote_address,
					dev_info->remote_name) >= 0) {
			_bt_update_device_list(ad);
		} else {
			if (dev != NULL) {
				/* Remove the item in searched dialogue */
				_bt_remove_searched_device_item(ad, dev);
			}

			/* Add the item in paired group */
			new_dev = _bt_create_paired_device_info(dev_info);
			if (new_dev == NULL) {
				ERR("Create new device item failed");
				return;
			}
			new_dev->ad = ad;

			item = _bt_add_paired_device_item(ad, new_dev);
			if (item == NULL) {
				ERR("Fail to add the paired device");
				return;
			}

			ad->paired_device =
				eina_list_append(ad->paired_device, new_dev);

			_bt_connect_device(ad, new_dev);
			ad->paired_item = item;
		}
	}

	ad->searched_item = NULL;

	elm_object_disabled_set(ad->scan_btn, EINA_FALSE);

	FN_END;
}


void _bt_cb_bonding_destroyed(int result, char *remote_address,
					void *user_data)
{
	FN_START;

	bt_app_data_t *ad = NULL;
	bt_dev_t *new_item = NULL;
	bt_dev_t *item = NULL;
	Eina_List *l = NULL;
	int i;

	retm_if(remote_address == NULL, "Invalid argument: param is NULL\n");
	retm_if(user_data == NULL, "Invalid argument: param is NULL\n");

	ad = (bt_app_data_t *)user_data;

	if (ad->op_status == BT_UNPAIRING)
		ad->op_status = BT_ACTIVATED;

	if (result != BT_ERROR_NONE) {
		DBG("Failed to unbond: [%d]", result);
		return;
	}

	EINA_LIST_FOREACH(ad->paired_device, l, item) {
		if (item == NULL)
			break;

		if (g_strcmp0(item->addr_str, remote_address) == 0) {
			new_item = calloc(1, sizeof(bt_dev_t));
			if (new_item == NULL)
				break;
			item->connected_mask = 0x00;

			memcpy(new_item, item, sizeof(bt_dev_t));

			if (item->uuids && item->uuid_count > 0) {
				new_item->uuids = g_new0(char *, item->uuid_count + 1);

				for (i = 0; i < item->uuid_count; i++) {
					new_item->uuids[i] = g_strdup(item->uuids[i]);
				}
			}

			new_item->uuid_count = item->uuid_count;

			_bt_remove_paired_device_item(ad, item);
			_bt_update_device_list(ad);

			if (_bt_add_searched_device_item(ad, new_item) != NULL) {
				ad->searched_device = eina_list_append(
						ad->searched_device, new_item);
				_bt_update_device_list(ad);
			}

			break;
		}
	}

	FN_END;
}

void _bt_cb_service_searched(int result, bt_device_sdp_info_s *sdp_info,
				void* user_data)
{
	FN_START;

	int i;
	bt_app_data_t *ad = NULL;
	bt_service_class_t service_mask = 0;
	bt_dev_t *item = NULL;

	ret_if(user_data == NULL);

	ad = (bt_app_data_t *)user_data;

	ad->op_status = BT_ACTIVATED;

	DBG("Result: %d", result);

	item = _bt_get_dev_info_by_address(ad->paired_device,
					sdp_info->remote_address);

	if (item == NULL)
		item = _bt_get_dev_info(ad->paired_device, ad->paired_item);

	if (item == NULL) {
		ad->waiting_service_response = FALSE;
		return;
	}

	item->status = BT_IDLE;
	_bt_update_genlist_item((Elm_Object_Item *)item->genlist_item);

	if (result == BT_ERROR_NONE) {
		_bt_util_get_service_mask_from_uuid_list(
						sdp_info->service_uuid,
						sdp_info->service_count,
						&service_mask);

		if (sdp_info->service_uuid != NULL && sdp_info->service_count > 0) {

			_bt_util_free_device_uuids(item);
			item->uuids = g_new0(char *, sdp_info->service_count + 1);

			for (i = 0; i < sdp_info->service_count; i++) {
				item->uuids[i] = g_strdup(sdp_info->service_uuid[i]);
			}

			item->uuid_count = sdp_info->service_count;
		}

		item->service_list = service_mask;

		if (ad->waiting_service_response == TRUE) {
			_bt_connect_device(ad, item);
		}
	} else {
		DBG("Failed to get the service list [%d]", result);
#if 0
		if (ad->waiting_service_response == TRUE) {
			if (ad->popup) {
				evas_object_del(ad->popup);
				ad->popup = NULL;
			}

			ad->popup = _bt_create_popup(ad->win_main, BT_STR_ERROR,
					BT_STR_UNABLE_TO_GET_THE_SERVICE_LIST,
					_bt_popup_del_cb, ad, 2);
			ad->back_cb = _bt_util_launch_no_event;

			btn = elm_button_add(ad->popup);
			elm_object_style_set(btn, "popup");
			elm_object_text_set(btn, BT_STR_CANCEL);
			elm_object_part_content_set(ad->popup, "button1", btn);
			evas_object_smart_callback_add(btn, "clicked",
				(Evas_Smart_Cb)_bt_popup_del_cb, ad);
			ea_object_event_callback_add(ad->popup, EA_CALLBACK_BACK,
					_bt_popup_del_cb, ad);

			evas_object_show(ad->popup);

		}
#endif
	}

	ad->waiting_service_response = FALSE;

	FN_END;
}


void _bt_cb_hid_state_changed(int result, bool connected,
			const char *remote_address,
			void *user_data)
{
	DBG("");
}

void _bt_cb_audio_state_changed(int result, bool connected,
				const char *remote_address,
				bt_audio_profile_type_e type,
				void *user_data)
{
	FN_START;

	bt_address_t address = { { 0 } };
	int connected_type;
	bt_app_data_t *ad = NULL;

	ret_if(user_data == NULL);

	ad = (bt_app_data_t *)user_data;

	_bt_unlock_display();
	DBG("Bluetooth Audio Event [%d] : %s", result,
			connected ? "Connected" : "Disconnected");

	if (type == BT_AUDIO_PROFILE_TYPE_AG) {
		DBG("Not to handle the AG connection status");
		return;
	}

	if (type == BT_AUDIO_PROFILE_TYPE_A2DP) {
		connected_type = BT_STEREO_HEADSET_CONNECTED;
		if (connected && result == BT_APP_ERROR_NONE)
			ad->a2dp_connected = true;
	} else {
		connected_type = BT_HEADSET_CONNECTED;
	}

	if (ad->launch_mode == BT_LAUNCH_CONNECT_HEADSET) {
		if (connected && result == BT_APP_ERROR_NONE) {
			_bt_destroy_app(ad);
			return;
		}

		if (ad->main_genlist == NULL) {
			if (_bt_initialize_view(ad) < 0) {
				ERR("_bt_initialize_view failed");
				_bt_destroy_app(ad);
			}
			return;
		}

	}

	DBG("remote_address : %s", remote_address);
	_bt_util_addr_string_to_addr_type(address.bd_addr, remote_address);

	__bt_ipc_update_connected_status(user_data, connected_type,
					connected, result, &address);
	FN_END;
}


void _bt_cb_adapter_name_changed(char *device_name, void *user_data)
{
	DBG("");
}

gboolean _bt_send_result(bt_app_data_t *ad, bool result)
{
	if (ad == NULL)
		return FALSE;

	int ret;
	service_h service = NULL;
	int reply = SERVICE_RESULT_FAILED;

	DBG("");

	if (ad->service == NULL) {
		ERR("Invalid param");
		return FALSE;
	}

	service_create(&service);
	if (service == NULL) {
		ERR("Service create failed");
		return FALSE;
	}

	if (result)
		reply = SERVICE_RESULT_SUCCEEDED;

	ret = service_reply_to_launch_request(service, ad->service, reply);
	if (ret != SERVICE_ERROR_NONE) {
		ERR("service_reply_to_launch_request() failed");
		service_destroy(service);
		return FALSE;
	}

	DBG("Send result : %d", reply);

	service_destroy(service);

	return TRUE;
}

gboolean _bt_init(void *data)
{
	int ret;
	bt_adapter_state_e adapter_state = BT_ADAPTER_DISABLED;
	bt_app_data_t *ad = (bt_app_data_t *)data;
	retv_if(!ad, FALSE);

	ret = bt_initialize();
	if (ret != BT_ERROR_NONE)
		ERR("bt_initialize is failed : %d", ret);

	ret = bt_audio_initialize();
	if (ret != BT_ERROR_NONE)
		ERR("bt_audio_initialize() failed");

	ret = bt_adapter_set_state_changed_cb(__bt_adapter_state_changed, data);
	if (ret != BT_ERROR_NONE)
		ERR("bt_adapter_set_state_changed_cb failed");

	ret =  bt_adapter_set_device_discovery_state_changed_cb(
			_bt_cb_discovery_state_changed, data);
	if (ret != BT_ERROR_NONE)
		ERR("bt_adapter_set_device_discovery_state_changed_cb failed : %d", ret);

	ret = bt_device_set_bond_created_cb(_bt_cb_bonding_created, data);
	if (ret != BT_ERROR_NONE)
		ERR("bt_device_set_bond_created_cb failed");

	ret = bt_device_set_bond_destroyed_cb(_bt_cb_bonding_destroyed,
					      data);
	if (ret != BT_ERROR_NONE)
		ERR("bt_device_set_bond_destroyed_cb failed");

	ret = bt_device_set_service_searched_cb(_bt_cb_service_searched,
						data);
	if (ret != BT_ERROR_NONE)
		ERR("bt_device_set_service_searched_cb failed");

	ret = bt_adapter_set_name_changed_cb(_bt_cb_adapter_name_changed,
					     data);
	if (ret != BT_ERROR_NONE)
		ERR("bt_adapter_set_name_changed_cb failed");

	ret =
	    bt_audio_set_connection_state_changed_cb(_bt_cb_audio_state_changed,
						     data);
	if (ret != BT_ERROR_NONE)
		ERR("audio set connection state callback failed");


	ret = bt_adapter_get_state(&adapter_state);
	if (ret != BT_ERROR_NONE) {
		ERR("bt_adapter_get_state is failed : %d", ret);
		return FALSE;
	}

	if (adapter_state == BT_ADAPTER_ENABLED) {
		DBG("Aleady BT enabled");
		return TRUE;
	} else {
		DBG("Enable BT adapter");
		ret = bt_adapter_enable();
		ad->op_status = BT_ACTIVATING;
		if (ret != BT_ERROR_NONE) {
			ERR("bt_adapter_enable is failed : %d", ret);
			return FALSE;
		}
	}

	if (ret != BT_ERROR_NONE)
		return FALSE;
	else
		return TRUE;
}

void _bt_deinit(bt_app_data_t *ad)
{
	FN_START;
	int ret;

	if (ad && ad->op_status == BT_SEARCHING) {
		ret = bt_adapter_stop_device_discovery();
		if (ret != BT_ERROR_NONE)
			ERR("Stop device discovery failed: %d", ret);
	}

	ret = bt_adapter_unset_state_changed_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_adapter_unset_state_changed_cb is failed : %d\n", ret);

	ret = bt_adapter_unset_device_discovery_state_changed_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_adapter_unset_device_discovery_state_changed_cb is failed : %d\n", ret);

	ret = bt_device_unset_bond_created_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_device_unset_bond_created_cb is failed : %d\n", ret);

	ret = bt_device_unset_bond_destroyed_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_device_unset_bond_destroyed_cb is failed : %d\n", ret);

	ret = bt_device_unset_service_searched_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_device_unset_service_searched_cb is failed : %d\n", ret);

	ret = bt_adapter_unset_name_changed_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_adapter_unset_name_changed_cb is failed : %d\n", ret);

	ret = bt_audio_unset_connection_state_changed_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_audio_unset_connection_state_changed_cb is failed : %d\n", ret);

	ret = bt_deinitialize();
	if (ret != BT_ERROR_NONE)
		ERR("bt_deinitialize is failed : %d\n", ret);

	FN_END;
	return;
}


