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

#include "um_main.h"
#include <heynoti.h>

static void fini(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	__USB_FUNC_EXIT__;
}

static void usb_server_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	um_signal_init();
	appcore_set_i18n(PACKAGE, LOCALEDIR);
	um_usb_server_init(ad);
	__USB_FUNC_EXIT__;
}

static int usb_server_main(int argc, char **argv)
{
	__USB_FUNC_ENTER__;
	UmMainData ad;
	memset(&ad, 0x0, sizeof(UmMainData));

	ecore_init();

	usb_server_init(&ad);
	if (USB_DEVICE_CLIENT == ad.isHostOrClient) {
		ad.usbAcc = (UsbAccessory*)malloc(sizeof(UsbAccessory));
	}

	ecore_main_loop_begin();

	fini(&ad);
	ecore_shutdown();
	FREE(ad.usbAcc);

	if (USB_DEVICE_HOST == ad.isHostOrClient) {
		if (USB_HOST_CONNECTED == check_usbhost_connection())
			return 1;
	} else if (USB_DEVICE_CLIENT == ad.isHostOrClient) {
		if (USB_CLIENT_CONNECTED == check_usbclient_connection())
			return 1;
	}

	__USB_FUNC_EXIT__;
	return 0;
}

static int elm_main(int argc, char **argv)
{
	__USB_FUNC_ENTER__;
	int ret = 0;
	while(1) {
		ret = usb_server_main(argc, argv);
		if (ret == 0) break;
	}
	__USB_FUNC_EXIT__;
	return 0;
}

int main(int argc, char **argv)
{
	__USB_FUNC_ENTER__;
	__USB_FUNC_EXIT__;
	return elm_main(argc, argv);
}
