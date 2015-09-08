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

#ifndef __BT_STRING_H__
#define __BT_STRING_H__

#include "bt-main.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef N_
#undef N_
#endif
#define N_(str)			gettext_noop(str)

#ifdef _
#undef _
#endif
#define _(str)			gettext(str)

#define STR_SCAN _("IDS_BT_SK_SCAN")
#define STR_STOP _("IDS_BT_SK_STOP")
#define STR_NO_DEV _("IDS_ST_BODY_NO_NEARBY_BLUETOOTH_DEVICES_FOUND")
#define STR_PAIRED_DEV _("IDS_BT_BODY_PAIRED_DEVICES")
#define STR_AVAILABLE_DEV _("IDS_BT_BODY_AVAILABLE_DEVICES")
#define STR_SCANNING _("IDS_BT_BODY_SCANNING_ING")
#define STR_PAIRED _("IDS_BT_BODY_PAIRED")
#define STR_PAIRING _("IDS_BT_BODY_PAIRING_ING")
#define STR_SERVICE_SEARCHING ""
#define STR_CONNECTED _("IDS_BT_POP_CONNECTED")
#define STR_CONNECT _("IDS_BT_BUTTON_CONNECT_ABB2")
#define STR_CONNECTING _("IDS_BT_BODY_CONNECTING")
#define STR_DISCONNECTING _("IDS_BT_BODY_DISCONNECTING")
#define STR_TITLE_DISCONNECT_Q _("IDS_BT_POP_DISCONNECT")
#define STR_TITLE_UNPAIR_Q _("IDS_BT_OPT_UNPAIR")
#define STR_DISCONNECT_DEV_Q _("IDS_WMGR_POP_THIS_WILL_END_YOUR_CONNECTION_WITH_PS")
#define STR_AUTOCONNECT_DEV _("IDS_BT_POP_CONNECTING_TO_BLUETOOTH_STEREO_HEADSET_ING")
#define STR_UNPAIR_DEV_Q _("IDS_ST_POP_PS_WILL_BE_UNPAIRED")
#define STR_TITLE_DISABLE_PROFILE _("IDS_ST_HEADER_DISABLE_PROFILE_ABB")
#define BT_STR_UNPAIR _("IDS_BT_OPT_UNPAIR")
#define BT_STR_PROFILE_OPTIONS _("IDS_ST_HEADER_PROFILES")
#define BT_STR_CALL_AUDIO _("IDS_BT_BODY_CALL_AUDIO")
#define BT_STR_MEDIA_AUDIO _("IDS_ST_MBODY_MEDIA_AUDIO_ABB")
#define BT_STR_DISABLE_PROFILE_OPTION _("IDS_ST_POP_THE_P1SS_PROFILE_WILL_BE_DISABLED_FOR_THE_P2SS")

/* System string */
#define STR_OK			_("IDS_BT_BUTTON_OK_ABB")
#define STR_CANCEL		_("IDS_BT_BUTTON_CANCEL")

#ifdef __cplusplus
}
#endif

#endif /* __BT_STRING_H__ */
