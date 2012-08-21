/*
 * USB server
 *
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Taeyoung Kim <ty317.kim@samsung.com>
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
 *
*/

#include "um_usb_accessory_manager.h"
#include <vconf.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stat.h>

int initAccessory(UsbAccessory *usbAcc)
{
	__USB_FUNC_ENTER__;
	if (!usbAcc) return -1;

	int acc = open(USB_ACCESSORY_NODE, O_RDONLY);
	um_retvm_if(acc < 0, -1, "FAIL: open(USB_ACCESSORY_NODE, O_RDONLY)");

	char buf[ACC_ELEMENT_LEN];
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

int accessoryAttached(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	load_system_popup(ad, SELECT_PKG_FOR_ACC_POPUP);
	__USB_FUNC_EXIT__;
	return 0;
}

int connectAccessory(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	int ret = -1;
	ret = initAccessory(ad->usbAcc);
	um_retvm_if(0 != ret, -1, "FAIL: initAccessory(ad->usbAcc)");
	getCurrentAccessory(ad);

	/* Change usb mode to accessory mode */
	ret = vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, SETTING_USB_ACCESSORY_MODE);
	um_retvm_if(0 != ret, -1, "FAIL: vconf_set_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT)");
	
	ret = accessoryAttached(ad);
	um_retvm_if(0 > ret, -1, "FAIL: accessoryAttached(ad);");

	__USB_FUNC_EXIT__;
	return 0;
}

int loadURIForAccessory(UsbAccessory *usbAcc)
{
	__USB_FUNC_ENTER__;
	if (!usbAcc) return -1;
/*	service_h service_handle = NULL;
	if (service_create(&service_handle) < 0) {
		USB_LOG("FAIL: service_create(&service_handle)");
		return -1;
	}
	um_retvm_if (!service_handle, -1, "service_handle");
	if (service_set_operation(service_handle, "http://tizen.org/appsvc/operation/view") < 0) {
		USB_LOG("FAIL: service_set_operation(service_handle, http://tizen.org/appsvc/operation/view)");
		service_destroy(service_handle);
		return -1;
	}
	if (service-set_uri(servoce_handle, usbAcc->uri) < 0) {
		USB_LOG("FAIL: service-set_uri(servoce_handle, usbAcc->uri)");	
		service_destroy(service_handle);
		return -1;
	}
	service_destroy(service_handle);*/
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

int grantAccessoryPermission(UmMainData *ad, char *appId)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	if (!appId) return -1;

	if (ad->permittedPkgForAcc != NULL) {
		USB_LOG("Previous permitted pkg is removed\n");
	}
	FREE(ad->permittedPkgForAcc);
	ad->permittedPkgForAcc = strdup(appId);
	USB_LOG("Permitted pkg for accessory is %s\n", ad->permittedPkgForAcc);
	__USB_FUNC_EXIT__;
	return 0;
}

int launch_app(char *appId)
{
	__USB_FUNC_ENTER__;
	if (appId == NULL) return -1;
	bundle *b = NULL;
	b = bundle_create();
	um_retvm_if(!b, -1, "FAIL: bundle_create()"); 
	int ret = aul_launch_app(appId, b);
	bundle_free(b);
	um_retvm_if(0 > ret, -1, "FAIL: aul_launch_app(appId, b)");
	__USB_FUNC_EXIT__;
	return 0;
}

Eina_Bool hasPermission(UmMainData *ad, char *appId)
{
	/* Check whether or not a package has permission to access to device/accessory */
	__USB_FUNC_ENTER__;
	if (!ad) return EINA_FALSE;
	if (ad->permittedPkgForAcc && appId) {
		if (!strncmp(ad->permittedPkgForAcc, appId, strlen(appId))) {
			__USB_FUNC_EXIT__;
			return EINA_TRUE;
		}
	}
	__USB_FUNC_EXIT__;
	return EINA_FALSE;
}

static int usbAccessoryRelease(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	FREE(ad->usbAcc->manufacturer);
	FREE(ad->usbAcc->version);
	FREE(ad->usbAcc->description);
	FREE(ad->usbAcc->model);
	FREE(ad->usbAcc->uri);
	FREE(ad->usbAcc->serial);
	FREE(ad->permittedPkgForAcc);

	__USB_FUNC_EXIT__;
	return 0;
}

void umAccInfoInit(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return ;
	int ret = usbAccessoryRelease(ad);
	if (0 != ret) USB_LOG("FAIL: usb_accessory_release(ad)\n");

	__USB_FUNC_EXIT__;
}

int disconnectAccessory(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	usbAccessoryRelease(ad);
	__USB_FUNC_EXIT__;
	return 0;
}

void getCurrentAccessory(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return ;
	if (!(ad->usbAcc)) return ;
	/* Get current accessory */
	USB_LOG("** USB Accessory Info **\n");
	USB_LOG("Manufacturer: %s\n", ad->usbAcc->manufacturer);
	USB_LOG("Model       : %s\n", ad->usbAcc->model);
	USB_LOG("Description : %s\n", ad->usbAcc->description);
	USB_LOG("Version     : %s\n", ad->usbAcc->version);
	USB_LOG("Uri         : %s\n", ad->usbAcc->uri);
	USB_LOG("Serial      : %s\n", ad->usbAcc->serial);
	USB_LOG("************************\n");
	__USB_FUNC_EXIT__;
}
