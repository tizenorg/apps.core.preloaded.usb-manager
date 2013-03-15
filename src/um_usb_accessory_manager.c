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

#include "um_usb_accessory_manager.h"
#include <vconf.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <aul.h>

#define USB_ACCESSORY_NODE "/dev/usb_accessory"

static void show_cur_accessory(UsbAccessory *acc)
{
	__USB_FUNC_ENTER__;
	assert(acc);

	/* Get current accessory */
	USB_LOG("** USB Accessory Info **");
	USB_LOG("Manufacturer: %s", acc->manufacturer);
	USB_LOG("Model       : %s", acc->model);
	USB_LOG("Description : %s", acc->description);
	USB_LOG("Version     : %s", acc->version);
	USB_LOG("Uri         : %s", acc->uri);
	USB_LOG("Serial      : %s", acc->serial);
	USB_LOG("************************");
	__USB_FUNC_EXIT__;
}

static int get_accessory_info(UsbAccessory *usbAcc)
{
	__USB_FUNC_ENTER__;
	assert(usbAcc);
	int acc;
	char buf[DEVICE_ELEMENT_LEN];

	acc = open(USB_ACCESSORY_NODE, O_RDONLY);
	um_retvm_if(acc < 0, -1, "FAIL: open(USB_ACCESSORY_NODE, O_RDONLY)");

	ioctl(acc, USB_ACCESSORY_GET_MANUFACTURER, buf);
	usbAcc->manufacturer = strdup(buf);
	ioctl(acc, USB_ACCESSORY_GET_MODEL, buf);
	usbAcc->model = strdup(buf);
	ioctl(acc, USB_ACCESSORY_GET_DESCRIPTION, buf);
	usbAcc->description = strdup(buf);
	ioctl(acc, USB_ACCESSORY_GET_VERSION, buf);
	usbAcc->version = strdup(buf);
	ioctl(acc, USB_ACCESSORY_GET_URI, buf);
	usbAcc->uri = strdup(buf);
	ioctl(acc, USB_ACCESSORY_GET_SERIAL, buf);
	usbAcc->serial = strdup(buf);

	close(acc);

	__USB_FUNC_EXIT__;
	return 0;
}

static int accessory_attached(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(ad->usbAcc);
	/* TODO
	 * Manifest should be used to find all related apps for usb accessory */
	/*
	int ret;

	ret = launch_usb_syspopup(ad, SELECT_PKG_FOR_ACC_POPUP, ad->usbAcc);
	if (0 > ret) {
		USB_LOG("FAIL: launch_usb_syspopup(SELECT_PKG_FOR_ACC_POPUP)");
		return -1;
	}
	*/

	__USB_FUNC_EXIT__;
	return 0;
}

static int connect_accessory(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	int ret;

	ret = get_accessory_info(ad->usbAcc);
	um_retvm_if(0 != ret, -1, "FAIL: getAccessoryInfo(ad->usbAcc)");

	show_cur_accessory(ad->usbAcc);

	/* Change usb mode to accessory mode */
	ret = vconf_set_int(VCONFKEY_USB_SEL_MODE, SET_USB_ACCESSORY);
	um_retvm_if(0 != ret, -1, "FAIL: vconf_set_int(USB_SEL_MODE, ACCESSORY)");

	ret = accessory_attached(ad);
	um_retvm_if(0 > ret, -1, "FAIL: accessoryAttached(ad);");

	__USB_FUNC_EXIT__;
	return 0;
}

int load_uri_for_accessory(UsbAccessory *usbAcc)
{
	__USB_FUNC_ENTER__;
	assert(usbAcc);
	/* TODO When unknown USB accessory is connected, URI should be shown */
	bundle *bd = bundle_create();
	um_retvm_if(NULL == bd, -1, "FAIL: bundle_create()");
	appsvc_set_operation(bd, APPSVC_OPERATION_VIEW);
	appsvc_set_uri(bd, usbAcc->uri);
	appsvc_run_service(bd, 0, NULL, NULL);
	if (bundle_free(bd)) {
		USB_LOG("FAIL: bundle_free()");
	}
	__USB_FUNC_EXIT__;
	return 0;
}

int grant_accessory_permission(UmMainData *ad, char *appId)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(appId);

	FREE(ad->permAccAppId);

	ad->permAccAppId = strdup(appId);
	if (!(ad->permAccAppId)) {
		USB_LOG("FAIL: strdup()");
		return -1;
	}

	USB_LOG("Permitted App id for accessory is %s", ad->permAccAppId);
	__USB_FUNC_EXIT__;
	return 0;
}

int launch_acc_app(char *appId)
{
	__USB_FUNC_ENTER__;
	assert(appId);

	bundle *b;
	int ret;

	b = bundle_create();
	um_retvm_if(!b, -1, "FAIL: bundle_create()");

	ret = aul_launch_app(appId, b);
	if (0 > ret) {
		USB_LOG("FAIL: aul_launch_app(appId, b)");
		if (0 > bundle_free(b))
			USB_LOG("FAIL: bundle_free(b)");
		return -1;
	}

	if (0 > bundle_free(b))
		USB_LOG("FAIL: bundle_free(b)");
	__USB_FUNC_EXIT__;
	return 0;
}

bool has_accessory_permission(UmMainData *ad, char *appId)
{
	/* Check whether or not a package has permission to access to device/accessory */
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(appId);
	if (ad->permAccAppId) {
		if (!strncmp(ad->permAccAppId, appId, strlen(appId))) {
			__USB_FUNC_EXIT__;
			return true;
		}
	}
	__USB_FUNC_EXIT__;
	return false;
}

void accessory_info_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	ad->permAccAppId = NULL;

	if (!(ad->usbAcc)) return;

	ad->usbAcc->manufacturer = NULL;
	ad->usbAcc->version = NULL;
	ad->usbAcc->description = NULL;
	ad->usbAcc->model = NULL;
	ad->usbAcc->uri = NULL;
	ad->usbAcc->serial = NULL;

	__USB_FUNC_EXIT__;
}

void disconnect_accessory(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	FREE(ad->permAccAppId);

	if (!(ad->usbAcc)) return;

	FREE(ad->usbAcc->manufacturer);
	FREE(ad->usbAcc->version);
	FREE(ad->usbAcc->description);
	FREE(ad->usbAcc->model);
	FREE(ad->usbAcc->uri);
	FREE(ad->usbAcc->serial);

	__USB_FUNC_EXIT__;
}

void um_uevent_usb_accessory_added(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int status;
	int ret;

	status = check_usbclient_connection();
	if (USB_CLIENT_CONNECTED != status)
		return;

	ret = vconf_get_int(VCONFKEY_USB_ACCESSORY_STATUS, &status);
	if (0 != ret) {
		USB_LOG("FAIL: vconf_get_int(VCONFKEY_USB_ACCESSORY_STATUS)");
		return;
	}

	if (VCONFKEY_USB_ACCESSORY_STATUS_DISCONNECTED != status) {
		__USB_FUNC_EXIT__;
		return;
	}

	ret = connect_accessory(ad);
	if (ret != 0) {
		USB_LOG("FAIL: connect_accessory(ad)");
		return ;
	}

	ret = vconf_set_int(VCONFKEY_USB_ACCESSORY_STATUS,
			VCONFKEY_USB_ACCESSORY_STATUS_CONNECTED);
	if (ret != 0)
		USB_LOG("FAIL: vconf_set_int(VCONFKEY_USB_ACCESSORY_STATUS)");

	__USB_FUNC_EXIT__;
}

