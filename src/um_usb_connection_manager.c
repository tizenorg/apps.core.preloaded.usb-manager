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

/* brad.t.peters@intel.com - todo - move away from sys-v init */
#define SDBD_START            "/sbin/sdbd"
#define SDBD_STOP             "/bin/killall sdbd"
#define SET_USB0_IP_ETHERNET  "/sbin/ifconfig usb0 192.168.129.3 up"
#define SET_USB0_IP_TETHERING SET_USB0_IP_ETHERNET /* This IP setting can be changed */
#define UNSET_USB0_IP         "/sbin/ifconfig usb0 down"
#define ADD_NETWORK_ID        "/sbin/route add -net 192.168.129.0 netmask 255.255.255.0 dev usb0"
#define OPENSSHD_START        "/etc/init.d/ssh start"
#define OPENSSHD_STOP         "/etc/init.d/ssh stop"

#define USB_NOTICE_SYSPOPUP_FAIL "USB system popup failed"

static int run_core_action(UmMainData *ad, int mode);
static int set_USB_mode(UmMainData *ad, int mode);

static bool get_current_debug_mode(void)
{
	__USB_FUNC_ENTER__ ;

	int ret;
	int debugMode;

	ret = vconf_get_bool(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL, &debugMode);
	if (ret == 0 && debugMode == 1) {
		__USB_FUNC_EXIT__ ;
		return true;
	}

	__USB_FUNC_EXIT__ ;
	return false;
}

static int get_default_mode(void)
{
	__USB_FUNC_ENTER__ ;

	if (get_current_debug_mode()) {
		__USB_FUNC_EXIT__ ;
		return SET_USB_SDB;
	}

	__USB_FUNC_EXIT__ ;
	return SET_USB_DEFAULT;
}

static int check_mobile_hotspot_status(void)
{
	__USB_FUNC_ENTER__ ;

	int mh_status;
	int ret;

	ret = vconf_get_int(VCONFKEY_MOBILE_HOTSPOT_MODE, &mh_status);
	if (0 != ret) {
		USB_LOG("FAIL: vconf_get_int(VCONFKEY_MOBILE_HOTSPOT_MODE)");
		return -1;
	}

	__USB_FUNC_EXIT__ ;
	return mh_status;
}

int connectUsbClient(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);

	int ret;
	int mh_status;
	int usbSelMode;
	bool debugMode = get_current_debug_mode();

	/* If the mobile hotspot is on, USB-setting changes USB mode to mobile hotspot */
	mh_status = check_mobile_hotspot_status();
	if ((mh_status >= 0) && (mh_status & VCONFKEY_MOBILE_HOTSPOT_MODE_USB)) {
		USB_LOG("Mobile hotspot is on");
		ret = vconf_set_int(VCONFKEY_USB_SEL_MODE, SET_USB_RNDIS_TETHERING);
		um_retvm_if (0 != ret, -1, "FAIL: vconf_set_int(VCONFKEY_USB_SEL_MODE)");
		__USB_FUNC_EXIT__ ;
		return 0;
	}

	ret = vconf_get_int(VCONFKEY_USB_SEL_MODE, &usbSelMode);
	if (0 != ret) {
		USB_LOG("FAIL: vconf_get_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT)");
		usbSelMode = SET_USB_DEFAULT;
	}

	switch (usbSelMode) {
	case SET_USB_DEFAULT:
		if (debugMode)
			usbSelMode = SET_USB_SDB;
		break;
	case SET_USB_SDB:
	case SET_USB_SDB_DIAG:
		if (!debugMode)
			usbSelMode = SET_USB_DEFAULT;
		break;
	case SET_USB_RNDIS:
		if (debugMode)
			usbSelMode = SET_USB_RNDIS_SDB;
		break;
	case SET_USB_RNDIS_SDB:
		if (!debugMode)
			usbSelMode = SET_USB_RNDIS;
		break;
	case SET_USB_RNDIS_TETHERING:
	case SET_USB_ACCESSORY:
	default:
		break;
	}

	ret = vconf_set_int(VCONFKEY_USB_SEL_MODE, usbSelMode);
	if (0 != ret) {
		USB_LOG("FAIL: vconf_set_int()");
		return -1;
	}

	change_mode_cb(NULL, ad);

	__USB_FUNC_EXIT__ ;
	return 0;
}

void disconnectUsbClient(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);

	int ret;
	int usbCurMode;
	int usbSelMode;

	ret = vconf_get_int(VCONFKEY_USB_CUR_MODE, &usbCurMode);
	if (ret == 0)
		action_clean(ad, usbCurMode);

	ret = vconf_set_int(VCONFKEY_USB_CUR_MODE, SET_USB_NONE);
	if (ret != 0)
		USB_LOG("ERROR: vconf_set_int(VCONFKEY_USB_CUR_MODE)");

	ret = vconf_get_int(VCONFKEY_USB_SEL_MODE, &usbSelMode);
	if (ret == 0) {
		switch (usbSelMode) {
		case SET_USB_RNDIS_TETHERING:
		case SET_USB_ACCESSORY:
			ret = vconf_set_int(VCONFKEY_USB_SEL_MODE, get_default_mode());
			if (ret < 0)
				USB_LOG("FAIL: vconf_set_int()");
			break;
		default:
			break;
		}
	}

	ret = vconf_set_int(VCONFKEY_SETAPPL_USB_IN_MODE_CHANGE, CHANGE_COMPLETE);
	if (0 != ret)
		USB_LOG("FAIL: vconf_set_int(VCONFKEY_SETAPPL_USB_IN_MODE_CHANGE)");

	__USB_FUNC_EXIT__ ;
}

static char *get_usb_connection_msg(int mode)
{
	__USB_FUNC_ENTER__ ;

	char *msg;

	switch(mode) {
	case SET_USB_DEFAULT:
	case SET_USB_SDB:
	case SET_USB_SDB_DIAG:
		msg = "IDS_COM_BODY_USB_CONNECTED";
		break;

	case SET_USB_RNDIS:
	case SET_USB_RNDIS_SDB:
		msg = "SSH enabled";
		break;

	case SET_USB_RNDIS_TETHERING:
		return NULL;

	case SET_USB_ACCESSORY:
		msg = "IDS_COM_BODY_CONNECTED_TO_A_USB_ACCESSORY";
		break;

	default:
		msg = USB_NOTICE_SYSPOPUP_FAIL;
		break;
	}

	__USB_FUNC_EXIT__ ;
	return strdup(msg);
}

#ifndef SIMULATOR
static void update_debug_mode_status(UmMainData *ad, int selMode)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);

	switch (selMode) {
	case SET_USB_DEFAULT:
	case SET_USB_RNDIS:
		if (0 != vconf_set_bool(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL, 0))
			USB_LOG("FAIL: vconf_set_bool()");
		ad->curDebugMode = false;
		ad->prevDebugMode = false;
		break;

	case SET_USB_SDB:
	case SET_USB_SDB_DIAG:
	case SET_USB_RNDIS_SDB:
		if (0 != vconf_set_bool(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL, 1))
			USB_LOG("FAIL: vconf_set_bool()");
		ad->curDebugMode = true;
		ad->prevDebugMode = true;
		break;

	case SET_USB_RNDIS_TETHERING:
	case SET_USB_ACCESSORY:
		USB_LOG("No change with debug mode");
		break;

	default:
		USB_LOG("ERROR: selMode(%d)", selMode);
		break;
	}
	__USB_FUNC_EXIT__ ;
}
#endif

static int usb_mode_change_done(UmMainData *ad, int done)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	int ret;
	int usbSelMode;
	int usbCurMode;
	char *msg;

	if (USB_CLIENT_DISCONNECTED == check_usbclient_connection()) {
		return 0;
	}

	ret = vconf_get_int(VCONFKEY_USB_SEL_MODE, &usbSelMode);
	if (0 > ret) {
		USB_LOG("FAIL: vconf_get_int(VCONFKEY_USB_SEL_MODE)");
		return -1;
	}

	ret = vconf_get_int(VCONFKEY_USB_CUR_MODE, &usbCurMode);
	if (0 > ret) {
		USB_LOG("FAIL: vconf_get_int(VCONFKEY_USB_CUR_MODE)");
		return -1;
	}
	USB_LOG("CurMode: %d, SelMode: %d", usbCurMode, usbSelMode);

	if (ACT_SUCCESS != done) { /* USB mode change failed */
		action_clean(ad, usbSelMode);
		ret = vconf_set_int(VCONFKEY_USB_CUR_MODE, SET_USB_NONE);
		if (0 > ret)
			USB_LOG("ERROR: vconf_set_int(VCONFKEY_USB_CUR_MODE)");
		launch_usb_syspopup(ad, ERROR_POPUP, NULL);
		return 0;
	}

	ret = vconf_set_int(VCONFKEY_USB_CUR_MODE, usbSelMode);
	if (0 > ret) {
		USB_LOG("FAIL: vconf_set_int(VCONFKEY_USB_CUR_MODE)");
		return -1;
	}

#ifndef SIMULATOR
	update_debug_mode_status(ad, usbSelMode);
#endif

	switch (usbSelMode) {
	case SET_USB_DEFAULT:
	case SET_USB_SDB:
	case SET_USB_SDB_DIAG:
		ret = vconf_set_int(VCONFKEY_SETAPPL_USB_MODE_INT, SETTING_USB_DEFAULT_MODE);
		if (ret < 0)
			USB_LOG("FAIL: vconf_set_int()");
		ret = vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, SETTING_USB_DEFAULT_MODE);
		if (ret < 0)
			USB_LOG("FAIL: vconf_set_int()");

		if (usbCurMode == SET_USB_DEFAULT
			|| usbCurMode == SET_USB_SDB
			|| usbCurMode == SET_USB_SDB_DIAG)
			return 0;
		break;

	case SET_USB_RNDIS:
	case SET_USB_RNDIS_SDB:
		ret = vconf_set_int(VCONFKEY_SETAPPL_USB_MODE_INT, SETTING_USB_ETHERNET_MODE);
		if (ret < 0)
			USB_LOG("FAIL: vconf_set_int()");
		ret = vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, SETTING_USB_ETHERNET_MODE);
		if (ret < 0)
			USB_LOG("FAIL: vconf_set_int()");

		if (usbCurMode == SET_USB_RNDIS
			|| usbCurMode == SET_USB_RNDIS_SDB)
			return 0;
		break;

	case SET_USB_RNDIS_TETHERING:
		ret = vconf_set_int(VCONFKEY_SETAPPL_USB_MODE_INT, SETTING_USB_TETHERING_MODE);
		if (ret < 0)
			USB_LOG("FAIL: vconf_set_int()");
		ret = vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, SETTING_USB_TETHERING_MODE);
		if (ret < 0)
			USB_LOG("FAIL: vconf_set_int()");

		/* USB tethering does not need noti popup, thus return directly */
		return 0;

	case SET_USB_ACCESSORY:
		ret = vconf_set_int(VCONFKEY_SETAPPL_USB_MODE_INT, SETTING_USB_ACCESSORY_MODE);
		if (ret < 0)
			USB_LOG("FAIL: vconf_set_int()");
		ret = vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, SETTING_USB_ACCESSORY_MODE);
		if (ret < 0)
			USB_LOG("FAIL: vconf_set_int()");
		break;

	case SET_USB_NONE:
		ret = vconf_set_int(VCONFKEY_SETAPPL_USB_MODE_INT, SETTING_USB_NONE_MODE);
		if (ret < 0)
			USB_LOG("FAIL: vconf_set_int()");
		ret = vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, SETTING_USB_NONE_MODE);
		if (ret < 0)
			USB_LOG("FAIL: vconf_set_int()");
		break;

	default:
		USB_LOG("usbSelMode is improper(%d)", usbSelMode);
		return 0;
	}

	msg = get_usb_connection_msg(usbSelMode);
	load_connection_popup(msg);
	FREE(msg);

	__USB_FUNC_EXIT__ ;
	return 0;
}

void change_prev_mode_cb(keynode_t* in_key, void *data)
{
	__USB_FUNC_ENTER__ ;
	assert(data);

	UmMainData *ad = (UmMainData *)data;
	int ret;
	int usbSelMode;
	int usbCurMode;
	int mode;
	bool debugMode = get_current_debug_mode();

	if (USB_CLIENT_DISCONNECTED == check_usbclient_connection()) {
		return ;
	}

	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, &usbSelMode);
	if (0 > ret) {
		USB_LOG("ERROR: Cannot get the vconf key");
		return;
	}

	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT, &usbCurMode);
	if (0 > ret) {
		USB_LOG("ERROR: Cannot get the vconf key");
		return;
	}

	if (usbCurMode == usbSelMode) {
		USB_LOG("Selected mode is same as current mode");
		return;
	}

	switch (usbSelMode) {
	case SETTING_USB_DEFAULT_MODE:
		if (debugMode)
			mode = SET_USB_SDB;
		else /* debugMode == false */
			mode = SET_USB_DEFAULT;
		break;

	case SETTING_USB_ETHERNET_MODE:
		if (debugMode)
			mode = SET_USB_RNDIS_SDB;
		else /* debugMode == false */
			mode = SET_USB_RNDIS;
		break;

	case SETTING_USB_TETHERING_MODE:
		mode = SET_USB_RNDIS_TETHERING;
		break;

	case SETTING_USB_ACCESSORY_MODE:
		mode = SET_USB_ACCESSORY;
		break;

	default:
		USB_LOG("FAIL: usbSelMode: %d", usbSelMode);
		return;
	}

#ifndef SIMULATOR
	ad->curDebugMode = debugMode;
#endif

	ret = vconf_set_int(VCONFKEY_USB_SEL_MODE, mode);
	if (ret < 0)
		USB_LOG("FAIL: vconf_set_int()");

	__USB_FUNC_EXIT__ ;
}

#ifndef SIMULATOR
void debug_mode_cb(keynode_t* in_key, void *data)
{
	__USB_FUNC_ENTER__ ;
	assert(data);

	UmMainData *ad = (UmMainData *)data;
	int ret;
	int usbCurMode;
	int usbSelMode;

	ad->curDebugMode = get_current_debug_mode();
	if (ad->curDebugMode == ad->prevDebugMode) {
		USB_LOG("Debug mode is not changed");
		return ;
	}

	ret = vconf_get_int(VCONFKEY_USB_CUR_MODE, &usbCurMode);
	if (0 > ret) {
		USB_LOG("FAIL: vconf_get_int()");
		return ;
	}

	switch (usbCurMode) {
	case SET_USB_DEFAULT:
		usbSelMode = SET_USB_SDB;
		break;

	case SET_USB_SDB:
	case SET_USB_SDB_DIAG:
		usbSelMode = SET_USB_DEFAULT;
		break;

	case SET_USB_RNDIS:
		usbSelMode = SET_USB_RNDIS_SDB;
		break;

	case SET_USB_RNDIS_SDB:
		usbSelMode = SET_USB_RNDIS;
		break;

	case SET_USB_RNDIS_TETHERING:
	case SET_USB_ACCESSORY:
		USB_LOG("Do nothing");
		return;

	default:
		USB_LOG("ERROR: usbCurMode: %d", usbCurMode);
		return ;
	}

	ret = vconf_set_int(VCONFKEY_USB_SEL_MODE, usbSelMode);
	if (0 > ret)
		USB_LOG("FAIL: vconf_set_int()");

	__USB_FUNC_EXIT__ ;
}
#endif

void change_mode_cb(keynode_t* in_key, void *data)
{
	__USB_FUNC_ENTER__ ;
	assert(data);

	UmMainData *ad = (UmMainData *)data;
	int ret;
	int usbSelMode;
	int usbCurMode;

	if (USB_CLIENT_DISCONNECTED == check_usbclient_connection()) {
		return ;
	}

#ifndef SIMULATOR
	ad->curDebugMode = get_current_debug_mode();
#endif

	ret = vconf_get_int(VCONFKEY_USB_SEL_MODE, &usbSelMode);
	if (0 > ret) {
		USB_LOG("ERROR: Cannot get the vconf key");
		return ;
	}

	ret = vconf_get_int(VCONFKEY_USB_CUR_MODE, &usbCurMode);
	if (0 > ret) {
		USB_LOG("ERROR: Cannot get the vconf key");
		return;
	}

	if (usbSelMode == usbCurMode) {
		USB_LOG("Previous connection mode is same as the input mode");
		return ;
	}

	ret = set_USB_mode(ad, usbSelMode);
	if (0 != ret) {
		USB_LOG("ERROR: Cannot set USB mode");
		return ;
	}

	__USB_FUNC_EXIT__ ;
	return ;
}

static int set_USB_mode(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);

	int ret;
	int done;
	int usbCurMode;

	ret = vconf_set_int(VCONFKEY_SETAPPL_USB_IN_MODE_CHANGE, IN_MODE_CHANGE);
	if (0 > ret) {
		USB_LOG("FAIL: vconf_set_int(VCONFKEY_SETAPPL_USB_IN_MODE_CHANGE)");
	}

	ret = vconf_get_int(VCONFKEY_USB_CUR_MODE, &usbCurMode);
	if (0 == ret && SET_USB_NONE != usbCurMode) {
		action_clean(ad, usbCurMode);
	}

	USB_LOG("Mode change : %d\n", mode);

	done = run_core_action(ad, mode);

	ret = usb_mode_change_done(ad, done);
	if(0 != ret) {
		USB_LOG("usb_mode_change_done(ad, done)");
		return -1;
	}

	ret = vconf_set_int(VCONFKEY_SETAPPL_USB_IN_MODE_CHANGE, CHANGE_COMPLETE);
	if (0 > ret)
		USB_LOG("vconf_set_int(VCONFKEY_SETAPPL_USB_IN_MODE_CHANGE)");

	__USB_FUNC_EXIT__ ;
	return 0;
}

void change_hotspot_status_cb(keynode_t* in_key, void *data)
{
	__USB_FUNC_ENTER__ ;

	int mh_status;
	int ret;
	int usbCurMode;

	if (USB_CLIENT_DISCONNECTED == check_usbclient_connection()) {
		return;
	}

	mh_status = check_mobile_hotspot_status();
	USB_LOG("mobile_hotspot_status: %d", mh_status);
	if (0 > mh_status) {
		USB_LOG("FAIL: Getting mobile hotspot status");
		return ;
	}

	ret = vconf_get_int(VCONFKEY_USB_CUR_MODE, &usbCurMode);
	if (0 > ret) {
		USB_LOG("FAIL: vconf_get_int(VCONFKEY_USB_CUR_MODE)");
		return ;
	}

	if (mh_status & VCONFKEY_MOBILE_HOTSPOT_MODE_USB) {
		USB_LOG("USB Mobile hotspot is on");
		if (usbCurMode == SET_USB_RNDIS_TETHERING)
			return;

		/* When mobile hotspot is on, this callabck function only sets
		 * vconf key value of USB mode. And then, the callback function
		 * for vconf change will be called */
		ret = vconf_set_int(VCONFKEY_USB_SEL_MODE, SET_USB_RNDIS_TETHERING);
		if (0 > ret)
			USB_LOG("FAIL: vconf_set_int(VCONFKEY_USB_SEL_MODE)");

	} else {
		USB_LOG("USB Mobile hotspot is off");
		if (usbCurMode != SET_USB_RNDIS_TETHERING)
			return;

		ret = vconf_set_int(VCONFKEY_USB_SEL_MODE, get_default_mode());
		if (0 > ret)
			USB_LOG("FAIL: vconf_set_int(VCONF_USB_SEL_MODE)");
	}
	__USB_FUNC_EXIT__ ;
}

/****************************************************/
/* Functions related to mode change                 */
/****************************************************/

static int run_core_action(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	int ret;

	ret = mode_set_kernel(ad, mode);
	if (0 != ret) {
		USB_LOG("FAIL : mode_set_kernel()");
		return ACT_FAIL;
	}

	switch(mode)
	{
	case SET_USB_DEFAULT:
		USB_LOG("Mode: SET_USB_DEFAULT (%d)", mode);
		call_cmd(CMD_DR_START);
		break;

	case SET_USB_SDB:
	case SET_USB_SDB_DIAG:
		USB_LOG("Mode: SET_USB_SDB(_DIAG) (%d)", mode);
		call_cmd(CMD_DR_START);

#ifndef SIMULATOR
		call_cmd(SDBD_START);
#endif

		break;

	case SET_USB_RNDIS:
	case SET_USB_RNDIS_SDB:
		USB_LOG("Mode: SET_USB_RNDIS(_DIAG) (%d)", mode);
		call_cmd(SET_USB0_IP_ETHERNET);
		call_cmd(ADD_NETWORK_ID);
		call_cmd(OPENSSHD_START);
		break;

	case SET_USB_RNDIS_TETHERING:
		USB_LOG("Mode: SET_USB_RNDIS_TETHERING (%d)", mode);
		call_cmd(SET_USB0_IP_TETHERING);
		call_cmd(ADD_NETWORK_ID);
		break;

	case SET_USB_ACCESSORY:
		USB_LOG("Mode: SET_USB_ACCESSORY (%d)", mode);
		break;

	default:
		break;
	}

	__USB_FUNC_EXIT__ ;
	return ACT_SUCCESS;
}

void action_clean(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	int ret;

	switch(mode)
	{
	case SET_USB_DEFAULT:
		USB_LOG("Mode: clean SET_USB_DEFAULT (%d)", mode);
		break;

	case SET_USB_SDB:
	case SET_USB_SDB_DIAG:
		USB_LOG("Mode: clean SET_USB_SDB(_DIAG) (%d)", mode);

#ifndef SIMULATOR
		call_cmd(SDBD_STOP);
#endif

		break;

	case SET_USB_RNDIS:
	case SET_USB_RNDIS_SDB:
		USB_LOG("Mode: clean SET_USB_RNDIS(_DIAG) (%d)", mode);
		call_cmd(OPENSSHD_STOP);
		call_cmd(UNSET_USB0_IP);
		break;

	case SET_USB_RNDIS_TETHERING:
		USB_LOG("Mode: clean SET_USB_RNDIS_TETHERING (%d)", mode);
		call_cmd(UNSET_USB0_IP);
		break;

	case SET_USB_ACCESSORY:
		USB_LOG("Mode: clean SET_USB_ACCESSORY (%d)", mode);
		break;

	default:
		break;
	}

	ret = mode_set_kernel(ad, SET_USB_NONE);
	if(0 != ret)
		USB_LOG("FAIL: mode_set_kernel(SET_USB_NONE)");

	__USB_FUNC_EXIT__ ;
}

/***********************************************/
/*  Functions related to popups                */
/***********************************************/

static void response_error_popup(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	int ret;

	ret = vconf_set_int(VCONFKEY_USB_SEL_MODE, get_default_mode());
	if(0 > ret)
		USB_LOG("ERROR: Cannot set the vconf key");

	__USB_FUNC_EXIT__ ;
}

void usb_connection_selected_btn(UmMainData *ad, int input)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);

	if (USB_CLIENT_DISCONNECTED == check_usbclient_connection()) {
		return;
	}

	switch (input) {
	case ERROR_POPUP_OK_BTN:
		USB_LOG("The button on the error popup is selected");
		response_error_popup(ad);
		break;

	default:
		break;
	}
	__USB_FUNC_EXIT__ ;
}
