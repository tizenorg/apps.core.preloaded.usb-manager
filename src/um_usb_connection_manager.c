/*
 * usb-manager
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "um_usb_connection_manager.h"

int call_cmd(char* cmd)
{
	__USB_FUNC_ENTER__ ;
	int ret = system(cmd);
	USB_LOG("The result of %s is %d\n",cmd, ret);
	__USB_FUNC_EXIT__ ;
	return ret;
}

int connectUsbClient(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	if(!ad) return -1;
	int ret = -1;
	int mh_status = -1;
	int usbSelMode = -1;
	int keep_ethernet = -1;

	/* If the mobile hotspot is on, USB-setting changes USB mode to mobile hotspot */
	mh_status = check_mobile_hotspot_status();
	if ((mh_status >= 0) && (mh_status & VCONFKEY_MOBILE_HOTSPOT_MODE_USB))
	{
		USB_LOG("Mobile hotspot is on\n");
		ret = vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, SETTING_USB_TETHERING_MODE);
		um_retvm_if (0 != ret, -1, "FAIL: vconf_set_int(VCONF_SETAPPL_USB_SEL_MODE_INT)\n");
		__USB_FUNC_EXIT__ ;
		return 0;
	} else {
		USB_LOG("Mobile hotspot is off\n");
	}

	/********************************************************************/
	/* Turn on ethernet mode if developers want to turn on the ethernet */
	ret = vconf_get_int(VCONFKEY_USB_KEEP_ETHERNET, &keep_ethernet);
	if (ret == 0 && keep_ethernet == VCONFKEY_USB_KEEP_ETHERNET_SET) {
		USB_LOG("Ethernet mode is loading");
		ret = vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, SETTING_USB_ETHERNET_MODE);
		if (ret < 0) USB_LOG("FAIL: vconf_set_int(VCONFKEY_SETAPPL_USB_MODE_INT)");
		__USB_FUNC_EXIT__ ;
		return 0;
	}
	/********************************************************************/

	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, &usbSelMode);
	if (0 != ret) {
		USB_LOG("FAIL: vconf_get_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT)\n");
		usbSelMode = SETTING_USB_DEFAULT_MODE;
	}

	ret = set_USB_mode(ad, usbSelMode);
	if (0 != ret) {
		USB_LOG("ERROR: Cannot set USB mode \n");
	}
	__USB_FUNC_EXIT__ ;
	return 0;
}

int disconnectUsbClient(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	if(!ad) return -1;
	int ret = -1;
	int usbCurMode = -1;

	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT, &usbCurMode);
	um_retvm_if(ret <0, -1, "FAIL: vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT)\n");

	action_clean(ad, usbCurMode);

	ret = vconf_set_int(VCONFKEY_SETAPPL_USB_MODE_INT, SETTING_USB_NONE_MODE);
	if (ret != 0) {
		USB_LOG("ERROR: vconf_set_int(VCONFKEY_SETAPPL_USB_MODE_INT)\n");
	}

	ret = vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, SETTING_USB_DEFAULT_MODE);
	if (0 != ret) {
		USB_LOG("ERROR: vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT)\n");
	}
	ret = vconf_set_int(VCONFKEY_SETAPPL_USB_IN_MODE_CHANGE, CHANGE_COMPLETE);
	if (0 != ret) {
		USB_LOG("FAIL: vconf_set_int(VCONFKEY_SETAPPL_USB_IN_MODE_CHANGE)\n");
	}
	__USB_FUNC_EXIT__ ;
	return 0;
}

char *get_usb_connection_msg()
{
	__USB_FUNC_ENTER__ ;
	char *msg = NULL;
	int usbCurMode = -1;
	int ret = vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT, &usbCurMode);
	um_retvm_if(0 != ret, NULL, "FAIL: vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT)");
	USB_LOG("usbCurMode: %d\n", usbCurMode);

	switch(usbCurMode) {
	case SETTING_USB_DEFAULT_MODE:
		msg = "IDS_COM_BODY_USB_CONNECTED";
		break;
	case SETTING_USB_ETHERNET_MODE:
		msg = "SSH enabled";
		break;
	case SETTING_USB_ACCESSORY_MODE:
		msg = "IDS_COM_BODY_CONNECTED_TO_A_USB_ACCESSORY";
		break;
	default:
		msg = USB_NOTICE_SYSPOPUP_FAIL;
		break;
	}

	__USB_FUNC_EXIT__ ;
	return strdup(msg);
}

int usb_mode_change_done(UmMainData *ad, int done)
{
	__USB_FUNC_ENTER__ ;
	if(!ad) return -1;
	int vconf_ret = -1;
	int ret = -1;
	int usbSelMode = -1;
	int usbCurMode = -1;
	char *msg = NULL;

	if (USB_CLIENT_DISCONNECTED == check_usbclient_connection()) {
		return 0;
	}

	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, &usbSelMode);
	um_retvm_if(ret < 0, -1, "FAIL: vconf_get_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT)\n");
	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT, &usbCurMode);
	um_retvm_if(ret < 0, -1, "FAIL: vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT)\n");
	USB_LOG("CurMode: %d, SelMode: %d", usbCurMode, usbSelMode);

	if(ACT_SUCCESS == done) {
		vconf_ret = vconf_set_int(VCONFKEY_SETAPPL_USB_MODE_INT, usbSelMode);
		um_retvm_if (0 != vconf_ret, -1, "FAIL: vconf_set_int(VCONFKEY_SETAPPL_USB_MODE_INT)\n");
#ifndef SIMULATOR
		ad->prevDebugMode = ad->curDebugMode;
#endif

		if (SETTING_USB_TETHERING_MODE == usbSelMode) {
			return 0;
		} else if (SETTING_USB_DEFAULT_MODE == usbSelMode
				&& SETTING_USB_DEFAULT_MODE == usbCurMode) {
			return 0;
		} else {
			msg = get_usb_connection_msg();
			load_connection_popup(ad, msg, TICKERNOTI_ORIENTATION_TOP);
			FREE(msg);
		}

	} else {	/* USB mode change failed */
		action_clean(ad, usbSelMode);
		load_system_popup(ad, ERROR_POPUP);
	}
	__USB_FUNC_EXIT__ ;
	return 0;
}

#ifndef SIMULATOR
void debug_mode_cb(keynode_t* in_key, void *data)
{
	__USB_FUNC_ENTER__ ;
	if(!data) return ;

	change_mode_cb(in_key, data);

	__USB_FUNC_EXIT__ ;
	return ;
}
#endif

#ifndef SIMULATOR
void change_mode_cb(keynode_t* in_key, void *data)
{
	__USB_FUNC_ENTER__ ;
	if(!data) return ;
	UmMainData *ad = (UmMainData *)data;
	int ret = -1;
	int usbSelMode = -1;
	int usbCurMode = -1;
	int debugMode = 0;

	if (USB_CLIENT_DISCONNECTED == check_usbclient_connection()) {
		return ;
	}

	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, &usbSelMode);
	um_retm_if (0 != ret , "ERROR: Cannot get the vconf key\n");
	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT, &usbCurMode);
	um_retm_if (0 != ret , "ERROR: Cannot get the vconf key\n");
	ret = vconf_get_bool(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL, &debugMode);
	um_retm_if (0 != ret , "ERROR: Cannot get the vconf key\n");
	USB_LOG("debugMode: %d", debugMode);
	if (debugMode == 1) ad->curDebugMode = true;
	else if (debugMode == 0) ad->curDebugMode = false;
	else {
		USB_LOG("ERROR: debugMode value is improper");
		return;
	}

	if (usbSelMode == SETTING_USB_DEFAULT_MODE
			&& usbCurMode == SETTING_USB_DEFAULT_MODE) {
		um_retm_if (ad->curDebugMode == ad->prevDebugMode, "Previous usb mode is same as the input mode");
	} else {
		um_retm_if (usbSelMode == usbCurMode, "Previous connection mode is same as the input mode");
	}
	um_retm_if (usbSelMode == usbCurMode, "Previous connection mode is same as the input mode");

	ret = set_USB_mode(ad, usbSelMode);
	um_retm_if (0 != ret, "ERROR: Cannot set USB mode \n");

	__USB_FUNC_EXIT__ ;
	return ;
}
#else
void change_mode_cb(keynode_t* in_key, void *data)
{
	__USB_FUNC_ENTER__ ;
	if(!data) return ;
	UmMainData *ad = (UmMainData *)data;
	int ret = -1;
	int usbSelMode = -1;
	int usbCurMode = -1;

	if (USB_CLIENT_DISCONNECTED == check_usbclient_connection()) {
		return ;
	}

	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, &usbSelMode);
	um_retm_if (0 != ret , "ERROR: Cannot get the vconf key\n");
	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT, &usbCurMode);
	um_retm_if (0 != ret , "ERROR: Cannot get the vconf key\n");
	um_retm_if (usbSelMode == usbCurMode, "Previous connection mode is same as the input mode");

	ret = set_USB_mode(ad, usbSelMode);
	um_retm_if (0 != ret, "ERROR: Cannot set USB mode \n");

	__USB_FUNC_EXIT__ ;
	return ;
}
#endif

int set_USB_mode(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__ ;
	if(!ad) return -1;
	int ret = -1;
	int done = ACT_FAIL;
	int usbCurMode = -1;

	ret = vconf_set_int(VCONFKEY_SETAPPL_USB_IN_MODE_CHANGE, IN_MODE_CHANGE);
	if (0 != ret) {
		USB_LOG("FAIL: vconf_set_int(VCONFKEY_SETAPPL_USB_IN_MODE_CHANGE)");
	}

	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT, &usbCurMode);
	if (0 == ret && SETTING_USB_NONE_MODE != usbCurMode) {
		action_clean(ad, usbCurMode);
	}
	USB_LOG("Mode change : %d\n", mode);
	done = run_core_action(ad, mode);
	ret = usb_mode_change_done(ad, done);
	um_retvm_if(0 != ret, -1, "usb_mode_change_done(ad, done)");

	ret = vconf_set_int(VCONFKEY_SETAPPL_USB_IN_MODE_CHANGE, CHANGE_COMPLETE);
	if (0 != ret) {
		USB_LOG("vconf_set_int(VCONFKEY_SETAPPL_USB_IN_MODE_CHANGE)");
	}

	__USB_FUNC_EXIT__ ;
	return 0;
}

void change_hotspot_status_cb(keynode_t* in_key, void *data)
{
	__USB_FUNC_ENTER__ ;
	if(!data) return ;

	UmMainData *ad = (UmMainData *)data;
	int mh_status = -1;
	int ret = -1;
	int usbCurMode = -1;
	if (USB_CLIENT_DISCONNECTED == check_usbclient_connection()) {
		return;
	}

	mh_status = check_mobile_hotspot_status();
	USB_LOG("mobile_hotspot_status: %d\n", mh_status);
	um_retm_if (0 > mh_status, "FAIL: Getting mobile hotspot status\n");

	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT, &usbCurMode);
	um_retm_if (0 != ret, "FAIL: vconf_get_int(VCONF_SETAPPL_USB_MODE_INT)\n");

	if (mh_status & VCONFKEY_MOBILE_HOTSPOT_MODE_USB) {
		USB_LOG("USB Mobile hotspot is on\n");

		if (usbCurMode != SETTING_USB_TETHERING_MODE) {
			/* When mobile hotspot is on, this callabck function only sets vconf key value of USB mode.
			 * And then, the callback function for vconf change will be called */
			ret = vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, SETTING_USB_TETHERING_MODE);
			um_retm_if (0 != ret, "FAIL: vconf_set_int(VCONF_SETAPPL_USB_MODE_INT)\n");
		}
	} else {
		USB_LOG("USB Mobile hotspot is off\n");
		if (usbCurMode == SETTING_USB_TETHERING_MODE) {
			ret = vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT,
							SETTING_USB_DEFAULT_MODE);
			if (0 != ret) {
				USB_LOG("FAIL: vconf_set_int(VCONF_SETAPPL_USB_SEL_MODE_INT)\n");
				return;
			}
		}
	}
	__USB_FUNC_EXIT__ ;
}

static int check_mobile_hotspot_status()
{
	__USB_FUNC_ENTER__ ;

	int mh_status = -1;
	int ret = -1;
	ret = vconf_get_int(VCONFKEY_MOBILE_HOTSPOT_MODE, &mh_status);
	um_retvm_if (0 != ret, -1, "FAIL: vconf_get_int(VCONFKEY_MOBILE_HOTSPOT_MODE)\n");
	__USB_FUNC_EXIT__ ;
	return mh_status;
}

/****************************************************/
/* Functions related to mode change                 */
/****************************************************/

static int run_core_action(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__ ;
	if(!ad) return ACT_FAIL;
	int ret = -1;

	switch(mode)
	{
	case SETTING_USB_DEFAULT_MODE:
		USB_LOG("Mode : SETTING_USB_DFAULT_MODE\n");
		ret = mode_set_kernel(ad, SETTING_USB_DEFAULT_MODE);
		um_retvm_if(0 != ret, ACT_FAIL, "FAIL : mode_set_kernel()\n");
		start_dr(ad);
#ifndef SIMULATOR
		if (ad->curDebugMode) {
			USB_LOG("Current Debug mode is on");
			call_cmd(SDBD_START);
		}
#endif
		break;

	case SETTING_USB_ETHERNET_MODE:
		USB_LOG("Mode: SETTING_USB_ETHERNET_MODE\n");
		ret = mode_set_kernel(ad, SETTING_USB_ETHERNET_MODE);
		um_retvm_if(0 != ret, ACT_FAIL, "FAIL : mode_set_kernel()\n");
		call_cmd(SET_USB0_IP);
		call_cmd(ADD_DEFAULT_GW);
		call_cmd(OPENSSHD_START);
		break;

	case SETTING_USB_TETHERING_MODE:
		USB_LOG("Mode : SETTING_USB_TETHERING_MODE\n");
		ret = mode_set_kernel(ad, SETTING_USB_TETHERING_MODE);
		um_retvm_if(0 != ret, ACT_FAIL, "FAIL : mode_set_kernel()\n");
		call_cmd(SET_USB0_IP);
		call_cmd(ADD_DEFAULT_GW);
		break;

	case SETTING_USB_ACCESSORY_MODE:
		USB_LOG("Mode : SETTING_USB_ACCESSORY_MODE\n");
		ret = mode_set_kernel(ad, SETTING_USB_ACCESSORY_MODE);
		um_retvm_if(0 != ret, ACT_FAIL, "FAIL : mode_set_kernel()\n");
		break;

	default:
		break;
	}
	__USB_FUNC_EXIT__ ;
	return ACT_SUCCESS;
}

int action_clean(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__ ;
	if(!ad) return -1;
	int ret = -1;

	switch(mode) {
	case SETTING_USB_DEFAULT_MODE:
		USB_LOG("Mode : SETTING_USB_DEFAULT_MODE action_clean\n");
#ifndef SIMULATOR
		if (ad->prevDebugMode) {
			USB_LOG("Previous debug mode was on");
			call_cmd(SDBD_STOP);
		}
#endif
		break;
	case SETTING_USB_ETHERNET_MODE:
		USB_LOG("Mode : SETTING_USB_ETHERNET_MODE action_clean\n");
		call_cmd(OPENSSHD_STOP);
		call_cmd(UNSET_USB0_IP);
		break;
	case SETTING_USB_TETHERING_MODE:
		USB_LOG("Mode : SETTING_USB_TETHERING_MODE action_clean\n");
		call_cmd(UNSET_USB0_IP);
		break;
	case SETTING_USB_ACCESSORY_MODE:
	default:
		break;
	}
	ret = mode_set_kernel(ad, SETTING_USB_NONE_MODE);
	um_retvm_if(0 != ret, -1, "FAIL: mode_set_kernel(SETTING_USB_NONE_MODE)");
	__USB_FUNC_EXIT__ ;
	return 0;
}

/***********************************************/
/*  Functions related to popups                */
/***********************************************/

void usb_connection_selected_btn(UmMainData *ad, int input)
{
	__USB_FUNC_ENTER__ ;
	if(!ad) return ;
	if (USB_CLIENT_DISCONNECTED == check_usbclient_connection()) {
		return;
	}

	switch (input) {
	case ERROR_POPUP_OK_BTN:
		USB_LOG("The button on the error popup is selected\n");
		response_error_popup(ad);
		break;
	default:
		break;
	}
	__USB_FUNC_EXIT__ ;
}

int response_error_popup(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	if(!ad) return -1;
	int ret = -1;
	ret = vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, SETTING_USB_DEFAULT_MODE);
	um_retvm_if(0 != ret, -1, "ERROR: Cannot set the vconf key\n");

	__USB_FUNC_EXIT__ ;
	return 0;
}
