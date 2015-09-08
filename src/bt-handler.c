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

	DBG("connected: %d, ad->connect_req %d , ad->disconnect_req %d",
			connected, ad->connect_req, ad->disconnect_req);

	ad->connect_req = FALSE;

	if (item == NULL)
		item = _bt_get_dev_info(ad->paired_device, ad->paired_item);

	if (item) {
		if (connected == TRUE) {
			item->status = BT_IDLE;
			item->connected_mask |= (result == BT_APP_ERROR_NONE) ? \
				connected_type : 0x00;

			bt_device_info_s *device_info = NULL;

			if (bt_adapter_get_bonded_device_info
				((const char *)item->addr_str,
						&device_info) != BT_ERROR_NONE) {
				if (device_info)
					bt_adapter_free_device_info(device_info);

				ERR("Get Bonded device info failed");
			}

			if (device_info == NULL) {
				ERR("device_info is NULL");
			} else {
				_bt_util_get_service_mask_from_uuid_list
					(device_info->service_uuid, device_info->service_count,
					&item->service_list);
				bt_adapter_free_device_info(device_info);
			}
		} else {
			if (!ad->disconnect_req)
				item->status = BT_IDLE;

			item->connected_mask &= (result == BT_APP_ERROR_NONE) ? \
				~connected_type : 0xFF;
		}

		if (item->connected_mask == 0x00)
			item->is_connected = 0;
		else
			item->is_connected = 1;

		if (item->status == BT_IDLE) {
			_bt_util_update_genlist_item(item->genlist_item);
			_bt_sort_paired_devices(ad, item, item->is_connected);
			_bt_util_disable_genlist_items(ad, EINA_FALSE);
		}

		_bt_destroy_popup(ad);
	}

#ifndef TELEPHONY_DISABLED
	retm_if (!ad->profile_vd, "profile_vd is NULL!");

	/* Check if the device update and the Profile view device is same */
	/* Go through the ugd->profile_vd->genlist and check device address */
	bt_dev_t *dev_info = (bt_dev_t *)elm_object_item_data_get(ad->profile_vd->name_item);

	/* dev_info can be NULL again, so a check is applied */
	retm_if (dev_info == NULL, "No device info");

	/* Match the BD address */
	ret_if (g_strcmp0(dev_info->addr_str, addr_str) != 0);

	dev_info->call_checked = dev_info->connected_mask & \
				BT_HEADSET_CONNECTED;

	dev_info->media_checked = dev_info->connected_mask & \
				BT_STEREO_HEADSET_CONNECTED;

	if(connected_type == BT_STEREO_HEADSET_CONNECTED) {
		ecore_idler_add(_bt_util_update_genlist_item, ad->profile_vd->media_item);

	} else {
		ecore_idler_add(_bt_util_update_genlist_item, ad->profile_vd->call_item);
	}
	_bt_util_set_list_disabled(ad->profile_vd, EINA_FALSE);
#endif

	FN_END;
}

static void __bt_adapter_state_changed(int result,
			bt_adapter_state_e adapter_state, void *user_data)
{
	retm_if (!user_data, "Invalid param");
	bt_app_data_t *ad = (bt_app_data_t *)user_data;
	retm_if (result != BT_ERROR_NONE, "BT Adapter operation is failed : %d", result);

	INFO("BT Adapter is %s", adapter_state == BT_ADAPTER_ENABLED ?
			"enabled" : "disabled");
	if (adapter_state == BT_ADAPTER_ENABLED) {
		ad->op_status = BT_ACTIVATED;
		if (ad->scan_btn)
			elm_object_disabled_set(ad->scan_btn, EINA_FALSE);
		if(!_bt_create_list_view(ad))
			ERR("_bt_create_list_view fail!");

		if (ad->do_auto_connect == FALSE) {
			_bt_auto_headset_connect(ad);
			ad->do_auto_connect = TRUE;
		}
	}
}

static bool __bt_cb_match_discovery_type(unsigned int major_class,
						unsigned int minor_class,
						unsigned int service_class,
						unsigned int mask)
{
	bt_device_major_mask_t major_mask = BT_DEVICE_MAJOR_MASK_MISC;

	if (mask == 0x000000)
		return TRUE;

	INFO("mask:[%x] service_class:[%x]", mask, service_class);

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
			if (minor_class != BTAPP_MIN_DEV_CLS_VIDEO_DISPLAY_AND_LOUD_SPEAKER)
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

	if (mask & major_mask)
		return TRUE;

	return FALSE;
}

static void __bt_cb_discovery_started(void *data)
{
	FN_START;
	_bt_lock_display();

	bt_app_data_t *ad = (bt_app_data_t *)data;
	ret_if(ad == NULL);
	ad->op_status = BT_SEARCHING;
	_bt_hide_no_devices(ad);
	if (elm_object_disabled_get(ad->scan_btn))
		elm_object_disabled_set(ad->scan_btn, EINA_FALSE);
	elm_object_text_set(ad->scan_btn, STR_STOP);

	_bt_remove_all_searched_devices_item(ad);

	if (ad->searched_title_item == NULL)
		_bt_create_group_title_item(ad, GROUP_SEARCH);

	_bt_update_genlist_item(ad->searched_title_item);
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
				info->bt_class.minor_device_class,
				info->bt_class.major_service_class_mask,
				ad->search_type) == FALSE) {
		DBG("No matched device type");
		return;
	}

	if (ad->launch_mode == BT_LAUNCH_CONNECT_HEADSET &&
		!(info->bt_class.major_service_class_mask & BT_COD_SC_RENDERING)) {
		DBG("A2DP is not supported. Do not add");
		return;
	}

	if (info->is_bonded == TRUE) {
		DBG("Bonded device");
		if (_bt_check_and_update_device(ad->paired_device,
					info->remote_address,
					info->remote_name) >= 0)
			return;
	}

	if (_bt_check_and_update_device(ad->searched_device,
					info->remote_address,
					info->remote_name) >= 0)
		return;

	dev = _bt_create_searched_device_info((void *)info);
	if (dev == NULL) {
		ERR("Create new device item failed ");
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
	if (elm_object_disabled_get(ad->scan_btn))
		elm_object_disabled_set(ad->scan_btn, EINA_FALSE);

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

	if (discovery_state == BT_ADAPTER_DEVICE_DISCOVERY_STARTED) {
		__bt_cb_discovery_started(user_data);
	} else if (discovery_state == BT_ADAPTER_DEVICE_DISCOVERY_FOUND)
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

	if (dev == NULL && ad->searched_item != NULL)
		dev = _bt_get_dev_info(ad->searched_device, ad->searched_item);

	if (result != BT_ERROR_NONE) {
		ERR("Failed to pair with device : Error Cause[%d]", result);
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

		INFO("Bonding completed successfully");

		if (_bt_check_and_update_device(ad->paired_device,
					dev_info->remote_address,
					dev_info->remote_name) < 0) {
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

			item = _bt_add_paired_device_item_on_bond(ad, new_dev);
			if (item == NULL) {
				ERR("Fail to add the paired device");
				return;
			}

			ad->paired_device =
				eina_list_append(ad->paired_device, new_dev);

			if (ad->searched_item != NULL)
				_bt_connect_device(ad, new_dev);
			ad->paired_item = item;
		}
	}

	ad->searched_item = NULL;

	if (ad->scan_btn)
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
		ERR("Failed to unbond: [%d]", result);
		return;
	}

	INFO("Bonding destroyed [%s]", remote_address);

	EINA_LIST_FOREACH(ad->paired_device, l, item) {
		if (item == NULL)
			break;

		if (g_strcmp0(item->addr_str, remote_address) == 0) {
			new_item = calloc(1, sizeof(bt_dev_t));
			if (new_item == NULL)
				break;
			item->connected_mask = 0x00;
			item->is_connected = 0x00;

			memcpy(new_item, item, sizeof(bt_dev_t));

			if (item->uuids && item->uuid_count > 0) {
				new_item->uuids = g_new0(char *, item->uuid_count + 1);

				for (i = 0; i < item->uuid_count; i++) {
					new_item->uuids[i] = g_strdup(item->uuids[i]);
				}
			}

			new_item->uuid_count = item->uuid_count;

			_bt_remove_paired_device_item(ad, item);

			if (_bt_add_searched_device_item(ad, new_item) != NULL) {
				ad->searched_device = eina_list_append(
						ad->searched_device, new_item);
			}
#ifndef TELEPHONY_DISABLED
			if (ad->profile_vd)
				_bt_delete_profile_view(new_item->ad);
#endif
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

	INFO("Result: %d", result);

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
		ERR("Failed to get the service list [%d]", result);
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
	INFO("Bluetooth Audio Event [%d] : %s", result,
			connected ? "Connected" : "Disconnected");

	if (type == BT_AUDIO_PROFILE_TYPE_AG) {
		DBG("Not to handle the AG connection status");
		return;
	}

	INFO("ad->launch_mode : %d, type : %d, connected : %d, result : %d",
		ad->launch_mode, type, connected, result);

	if (type == BT_AUDIO_PROFILE_TYPE_A2DP) {
		connected_type = BT_STEREO_HEADSET_CONNECTED;
		if (connected && result == BT_APP_ERROR_NONE)
			ad->a2dp_connected = true;
		else
			ad->a2dp_connected = false;
	} else {
		connected_type = BT_HEADSET_CONNECTED;
	}

	if (ad->launch_mode == BT_LAUNCH_CONNECT_HEADSET || ad->launch_mode == BT_LAUNCH_CALL) {
		if (connected && result == BT_APP_ERROR_NONE) {
			if (ad->launch_mode == BT_LAUNCH_CONNECT_HEADSET) {
				if (type == BT_AUDIO_PROFILE_TYPE_A2DP)
					_bt_destroy_app(ad);
				return;
			} else if (ad->launch_mode == BT_LAUNCH_CALL) {
				if (type == BT_AUDIO_PROFILE_TYPE_HSP_HFP)
					_bt_destroy_app(ad);
				return;
			}
		}
		if (ad->launch_mode == BT_LAUNCH_CALL || ad->main_genlist == NULL) {
			if (_bt_initialize_view(ad) < 0) {
				ERR("_bt_initialize_view failed");
				_bt_destroy_app(ad);
			}
			return;
		}
	}

	_bt_util_addr_string_to_addr_type(address.bd_addr, remote_address);

	__bt_ipc_update_connected_status(user_data, connected_type,
					connected, result, &address);
	FN_END;
}

void _bt_cb_device_state_changed(bool connected,
						bt_device_connection_info_s *conn_info,
						void *user_data)
{
	FN_START;
	retm_if(user_data == NULL, "Invalid argument: bt_app_data_t is NULL");

	bt_app_data_t *ad = NULL;
	bt_dev_t *dev = NULL;

	ad = (bt_app_data_t *)user_data;
	retm_if(ad == NULL, "Invalid argument: ad info is NULL");

	dev = _bt_get_dev_info_by_address(ad->paired_device, conn_info->remote_address);
	retm_if(dev == NULL, "Invalid argument: dev info is NULL");

	if (!connected && ad->disconnect_req == true) {
		ad->disconnect_req = false;
		dev->status = BT_IDLE;

		_bt_util_disable_genlist_items(ad, EINA_FALSE);
		_bt_sort_paired_devices(ad, dev, dev->is_connected);
		_bt_update_genlist_item((Elm_Object_Item *) dev->genlist_item);
	}
	FN_END;
}

void _bt_auto_headset_connect(bt_app_data_t *ad)
{
	DBG("+");
	if (ad->launch_mode == BT_LAUNCH_CONNECT_HEADSET || ad->launch_mode == BT_LAUNCH_CALL) {
		if (_bt_get_paired_device_count(ad) == 1) {
			DBG("Launch mode is %d", ad->launch_mode);

			bt_dev_t *dev = eina_list_nth(ad->paired_device, 0);
			if(dev == NULL) {
				ERR("dev is NULL");
				_bt_destroy_app(ad);
				return;
			}

			/* Check whether the only paired device is Headset */
			if (dev->service_list & BT_SC_HFP_SERVICE_MASK)
				DBG("Remote device support HFP");
			if (dev->service_list & BT_SC_A2DP_SERVICE_MASK)
				DBG("Remote device support A2DP");

			if (dev->major_class == BT_MAJOR_DEV_CLS_AUDIO &&
				(dev->service_list & BT_SC_A2DP_SERVICE_MASK) > 0) {
				DBG("Paired Item is Headset");
				if ((dev->connected_mask & BT_STEREO_HEADSET_CONNECTED) == 0) {
					INFO("Paired device is 1. Autoconnecting [%s]",
							dev->name);
					_bt_connect_device(ad, dev);
					INFO("dev->status : %d", dev->status);
					if (dev->status == BT_CONNECTING) {
						_bt_create_autoconnect_popup(dev);
						return;
					}
				}
			}
		}
	}

	if (_bt_initialize_view(ad) < 0) {
		ERR("_bt_initialize_view failed");
		_bt_destroy_app(ad);
	}
	DBG("-");
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
	app_control_h service = NULL;
	int reply = APP_CONTROL_RESULT_FAILED;

	DBG("");

	if (ad->service == NULL) {
		ERR("Invalid param");
		return FALSE;
	}

	app_control_create(&service);
	if (service == NULL) {
		ERR("Service create failed");
		return FALSE;
	}

	if (result)
		reply = APP_CONTROL_RESULT_SUCCEEDED;

	ret = app_control_reply_to_launch_request(service, ad->service, reply);
	if (ret != APP_CONTROL_ERROR_NONE) {
		ERR("app_control_reply_to_launch_request() failed");
		app_control_destroy(service);
		return FALSE;
	}
	INFO("Send result : %d", reply);
	app_control_destroy(service);

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

	ret =
	    bt_device_set_connection_state_changed_cb(_bt_cb_device_state_changed,
						     data);
	if (ret != BT_ERROR_NONE)
		ERR("device set connection state callback failed");


	ret = bt_adapter_get_state(&adapter_state);
	if (ret != BT_ERROR_NONE) {
		ERR("bt_adapter_get_state is failed : %d", ret);
		return FALSE;
	}

	if (adapter_state == BT_ADAPTER_ENABLED) {
		DBG("Aleady BT enabled");
		ad->do_auto_connect = TRUE;
		return TRUE;
	} else {
		DBG("Enable BT adapter");
		ret = bt_adapter_enable();

		ad->do_auto_connect = FALSE;
		ad->op_status = BT_ACTIVATING;
		if (ret != BT_ERROR_NONE) {
			ERR("bt_adapter_enable is failed : %d", ret);
			return FALSE;
		}
	}

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
		ERR("bt_adapter_unset_state_changed_cb is failed : %d", ret);

	ret = bt_adapter_unset_device_discovery_state_changed_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_adapter_unset_device_discovery_state_changed_cb is failed : %d", ret);

	ret = bt_device_unset_bond_created_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_device_unset_bond_created_cb is failed : %d", ret);

	ret = bt_device_unset_bond_destroyed_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_device_unset_bond_destroyed_cb is failed : %d", ret);

	ret = bt_device_unset_service_searched_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_device_unset_service_searched_cb is failed : %d", ret);

	ret = bt_adapter_unset_name_changed_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_adapter_unset_name_changed_cb is failed : %d", ret);

	ret = bt_audio_unset_connection_state_changed_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_audio_unset_connection_state_changed_cb is failed : %d", ret);

	ret = bt_device_unset_connection_state_changed_cb();
	if (ret != BT_ERROR_NONE)
		ERR("bt_device_unset_connection_state_changed_cb is failed : %d", ret);

	ret = bt_deinitialize();
	if (ret != BT_ERROR_NONE)
		ERR("bt_deinitialize is failed : %d", ret);

	FN_END;
	return;
}


