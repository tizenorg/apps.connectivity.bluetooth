/*
* ug-setting-bluetooth-efl
*
* Copyright 2012 Samsung Electronics Co., Ltd
*
* Contact: Hocheol Seo <hocheol.seo@samsung.com>
*           GirishAshok Joshi <girish.joshi@samsung.com>
*           DoHyun Pyun <dh79.pyun@samsung.com>
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

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-bindings.h>

#include "bt-main.h"
#include "bt-type-define.h"
#include "bt-util.h"
#include "bt-dbus-method.h"


/**********************************************************************
*                                                 Static Functions
***********************************************************************/

int __bt_get_adapter_path(DBusGConnection *GConn, char *path)
{
	FN_START;

	GError *error = NULL;
	DBusGProxy *manager_proxy = NULL;
	char *adapter_path = NULL;
	int ret = BT_APP_ERROR_NONE;
	gsize len = 0;

	retv_if(GConn == NULL, -1);
	retv_if(path == NULL, -1);

	manager_proxy = dbus_g_proxy_new_for_name(GConn, BLUEZ_DBUS_NAME, "/",
			"org.bluez.Manager");

	retv_if(manager_proxy == NULL, -1);

	dbus_g_proxy_call(manager_proxy, "DefaultAdapter", &error,
			G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &adapter_path,
			G_TYPE_INVALID);

	if (error != NULL) {
		DBG("Getting DefaultAdapter failed: [%s]", error->message);
		g_error_free(error);
		g_object_unref(manager_proxy);
		return -1;
	}

	if (adapter_path == NULL) {
		g_object_unref(manager_proxy);
		return -1;
	}

	len = g_strlcpy(path, adapter_path, BT_ADAPTER_PATH_LEN);

	if (len >= BT_ADAPTER_PATH_LEN) {
		DBG("The copied len is too large");
		ret = -1;
	}

	g_object_unref(manager_proxy);
	g_free(adapter_path);

	FN_END;
	return ret;
}


/**********************************************************************
*                                                Common Functions
***********************************************************************/

DBusGProxy *_bt_get_adapter_proxy(DBusGConnection *conn)
{
	FN_START;
	DBusGProxy *adapter = NULL;
	char adapter_path[BT_ADAPTER_PATH_LEN] = { 0 };

	retv_if(conn == NULL, NULL);

	if (__bt_get_adapter_path(conn, adapter_path) < 0) {
		DBG("Could not get adapter path");
		return NULL;
	}

	adapter = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME,
								adapter_path, ADAPTER_INTERFACE);

	FN_END;
	return adapter;
}

gboolean _bt_is_profile_connected(int connected_type,
				DBusGConnection *conn,
				unsigned char *addr)
{
	FN_START;
	char *object_path = NULL;
	char addr_str[BT_ADDRESS_STR_LEN + 1] = { 0 };
	gboolean connected = FALSE;
	DBusGProxy *proxy = NULL;
	DBusGProxy *adapter = NULL;
	GError *error = NULL;
	GHashTable *hash = NULL;
	GValue *value = NULL;
	char *interface = NULL;
	char path[BT_ADAPTER_PATH_LEN + 1] = {0};

	retv_if(conn == NULL, FALSE);
	retv_if(addr == NULL, FALSE);

	_bt_util_addr_type_to_addr_string(addr_str, addr);

	if (connected_type == BT_NETWORK_SERVER_CONNECTED) {
		if (__bt_get_adapter_path(conn, path) != BT_APP_ERROR_NONE)
			return FALSE;

		object_path = g_strdup(path);
	} else {
		adapter = _bt_get_adapter_proxy(conn);

		retv_if(adapter == NULL, FALSE);

		dbus_g_proxy_call(adapter, "FindDevice",
				  &error, G_TYPE_STRING, addr_str,
				  G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH,
				  &object_path, G_TYPE_INVALID);

		g_object_unref(adapter);

		if (error != NULL) {
			DBG("Failed to Find device: %s", error->message);
			g_error_free(error);
			return FALSE;
		}
	}

	retv_if(object_path == NULL, FALSE);

	switch (connected_type) {
	case BT_HEADSET_CONNECTED:
		interface = HEADSET_INTERFACE;
		break;
	case BT_STEREO_HEADSET_CONNECTED:
		interface = SYNK_INTERFACE;
		break;
	case BT_HID_CONNECTED:
		interface = HID_INTERFACE;
		break;
	case BT_NETWORK_CONNECTED:
		interface = NETWORK_INTERFACE;
		break;
	case BT_NETWORK_SERVER_CONNECTED:
		interface = NETWORK_SERVER_INTERFACE;
		break;
	default:
		DBG("Unknown type!");
		g_free(object_path);
		return FALSE;
	}

	DBG("Interface: %s", interface);

	proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, object_path, interface);

	g_free(object_path);

	retv_if(proxy == NULL, FALSE);

	if (connected_type == BT_NETWORK_SERVER_CONNECTED) {
		dbus_g_proxy_call(proxy, "GetProperties", &error,
					G_TYPE_STRING, addr_str,
					G_TYPE_INVALID,
					dbus_g_type_get_map("GHashTable",
						G_TYPE_STRING, G_TYPE_VALUE),
					&hash, G_TYPE_INVALID);

	} else {

		dbus_g_proxy_call(proxy, "GetProperties", &error,
					G_TYPE_INVALID,
					dbus_g_type_get_map("GHashTable",
						G_TYPE_STRING, G_TYPE_VALUE),
					&hash, G_TYPE_INVALID);
	}

	if (error != NULL) {
		DBG("Failed to get properties: %s", error->message);
		g_error_free(error);
		g_object_unref(proxy);
		return FALSE;
	}

	if (hash != NULL) {
		value = g_hash_table_lookup(hash, "Connected");
		connected = value ? g_value_get_boolean(value) : FALSE;
	}

	g_object_unref(proxy);
	FN_END;
	return connected;
}
