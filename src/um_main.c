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

#include <sys/file.h>
#include "um_main.h"

#define LOCK_USB_MANAGER "/tmp/lock_usb_manager"

static void usb_server_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);
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

	ecore_main_loop_begin();

	ecore_shutdown();

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

static int um_lock_usb_manager(void)
{
	__USB_FUNC_ENTER__ ;
	int fd;
	int ret;

	fd = open(LOCK_USB_MANAGER, O_CREAT | O_RDWR | O_CLOEXEC, 0600);
	if (fd == -1) {
		USB_LOG("FAIL: open(%s)", LOCK_USB_MANAGER);
		return -1;
	}

	ret = flock(fd, LOCK_EX | LOCK_NB);
	if (ret == -1) {
		USB_LOG("FAIL: flock(fd, LOCK_EX | LOCK_NB), errno: %d", errno);
		close(fd);
		return -1;
	}

	__USB_FUNC_EXIT__ ;
	return fd;
}

static int um_unlock_usb_manager(int fd)
{
	__USB_FUNC_ENTER__ ;
	int ret;

	if (fd == -1) return -1;

	ret = flock(fd, LOCK_UN);
	if (ret == -1) {
		USB_LOG("FAIL: flock(fd, LOCK_UN)");
		close(fd);
		return -1;
	}
	close(fd);

	__USB_FUNC_EXIT__ ;
	return 0;
}


static int elm_main(int argc, char **argv)
{
	__USB_FUNC_ENTER__;
	int ret;
	int fd;

	fd = um_lock_usb_manager();
	if (fd == -1) {
		USB_LOG("FAIL: um_lock_usb_manager()");
		return -1;
	}

	if (pm_lock_state(LCD_OFF, STAY_CUR_STATE, 0) != 0)
		USB_LOG("FAIL: pm_lock_state(LCD_OFF, STAY_CUR_STATE)");

	while(1) {
		ret = usb_server_main(argc, argv);
		if (ret == 0) break;
	}

	if (pm_unlock_state(LCD_OFF, STAY_CUR_STATE) != 0)
		USB_LOG("FAIL:pm_unlock_state(LCD_OFF, STAY_CUR_STATE)");

	ret = um_unlock_usb_manager(fd);
	if (ret < 0) USB_LOG("FAIL: um_unlock_usb_manager(fd)");

	__USB_FUNC_EXIT__;
	return 0;
}

int main(int argc, char **argv)
{
	__USB_FUNC_ENTER__;
	__USB_FUNC_EXIT__;
	return elm_main(argc, argv);
}
