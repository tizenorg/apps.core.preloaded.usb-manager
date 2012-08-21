/*
 * Usb Server
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


#include <vconf.h>
#include <devman.h>

#include "um_common.h"

int check_usb_connection()
{
	__USB_FUNC_ENTER__ ;
	int status = -1;
	int ret = -1;
	ret = vconf_get_int(VCONFKEY_SYSMAN_USB_STATUS, &status);
	um_retvm_if(0 != ret, -1, "FAIL: vconf_get_int(VCONFKEY_SYSMAN_USB_STATUS)");
	__USB_FUNC_EXIT__ ;
	return status;
}

int launch_usb_syspopup(UmMainData *ad, POPUP_TYPE _popup_type)
{
	__USB_FUNC_ENTER__ ;
	if (!ad) return -1;
	int ret = -1;
	char syspopup_key[SYSPOPUP_PARAM_LEN];
	char syspopup_value[ACC_ELEMENT_LEN];

	bundle *b = NULL;
	b = bundle_create();
	um_retvm_if (!b, -1, "FAIL: bundle_create()\n");

	snprintf(syspopup_key, SYSPOPUP_PARAM_LEN, "%d", SYSPOPUP_TYPE);
	snprintf(syspopup_value, ACC_ELEMENT_LEN, "%d", _popup_type);
	ret = bundle_add(b, syspopup_key, syspopup_value);
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add()\n");
		if (0 != bundle_free(b)) USB_LOG("FAIL: bundle_free()\n");
		return -1;
	}

	if (SELECT_PKG_FOR_ACC_POPUP == _popup_type) {
		int i;
		char device[ACC_INFO_NUM][ACC_ELEMENT_LEN];
		snprintf(device[ACC_MANUFACTURER], ACC_ELEMENT_LEN, "%s", ad->usbAcc->manufacturer);
		snprintf(device[ACC_MODEL], ACC_ELEMENT_LEN, "%s", ad->usbAcc->model);
		snprintf(device[ACC_DESCRIPTION], ACC_ELEMENT_LEN, "%s", ad->usbAcc->description);
		snprintf(device[ACC_VERSION], ACC_ELEMENT_LEN, "%s", ad->usbAcc->version);
		snprintf(device[ACC_URI], ACC_ELEMENT_LEN, "%s", ad->usbAcc->uri);
		snprintf(device[ACC_SERIAL], ACC_ELEMENT_LEN, "%s", ad->usbAcc->serial);
		for ( i = 0; i < ACC_INFO_NUM ; i++) {
			snprintf(syspopup_key, SYSPOPUP_PARAM_LEN, "%d", 1 + i);
			snprintf(syspopup_value, ACC_ELEMENT_LEN, "%s", device[i]);
			USB_LOG("key: %s, value: %s\n", syspopup_key, syspopup_value);
			ret = bundle_add(b, syspopup_key, syspopup_value);
			if (0 != ret) {
				USB_LOG("FAIL: bundle_add()\n");
				if (0 != bundle_free(b)) USB_LOG("FAIL: bundle_free()\n");
				return -1;
			}
		}
	}

	ret = syspopup_launch(USB_SYSPOPUP, b);
	if (0 > ret) {
		USB_LOG("FAIL: syspopup_launch() returns %d\n", ret);
		if (0 != bundle_free(b)) USB_LOG("FAIL: bundle_free()\n");
		return -1;
	}

	ret = bundle_free(b);
	um_retvm_if (0 != ret, -1, "FAIL: bundle_free()\n");

	__USB_FUNC_EXIT__ ;
	return 0;
}

void load_system_popup(UmMainData *ad, POPUP_TYPE _popup_type)
{
	__USB_FUNC_ENTER__ ;
	if (!ad) return ;
	int ret = -1;
	ret = launch_usb_syspopup(ad, _popup_type);
	if (0 != ret) {
		USB_LOG("FAIL: launch_usb_syspopup(_popup_type)\n");
	}
	__USB_FUNC_EXIT__ ;
}

int ipc_request_server_init()
{
	__USB_FUNC_ENTER__ ;
	int len, t;
	int sockFd = -1;
	struct sockaddr_un local;

	if ((sockFd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		USB_LOG("FAIL: socket(AF_UNIX, SOCK_STREAM, 0)");
		return -1;
	}
	local.sun_family = AF_UNIX;
	strncpy(local.sun_path, SOCK_PATH, strlen(SOCK_PATH)+1);
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family);

	if (bind(sockFd, (struct sockaddr *)&local, len) == -1) {
		perror("bind");
		USB_LOG("FAIL: bind((*sock_local), (struct sockaddr *)&local, len)");
		return -1;
	}
	chown(SOCK_PATH, 5000, 5000);
	chmod(SOCK_PATH, 0777);
	if (listen(sockFd, 5) == -1) {
		perror("listen");
		USB_LOG("FAIL: listen((*sock_local), 5)");
		return -1;
	}
	__USB_FUNC_EXIT__ ;
	return sockFd;
}

int ipc_request_server_close(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	if (ad->server_sock_local > 0)
		close(ad->server_sock_local);
	if (ad->server_sock_remote > 0)
		close(ad->server_sock_remote);
	__USB_FUNC_EXIT__ ;
	return 0;
}


/* This function initializes socket for ipc with usb-server */
int ipc_noti_server_init()
{
	__USB_FUNC_ENTER__ ;
	int len;
	int sock_remote;
	struct sockaddr_un remote;
	if ((sock_remote = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		USB_LOG("FAIL: socket(AF_UNIX, SOCK_STREAM, 0)");
		return -1;;
	}
	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, ACC_SOCK_PATH, strlen(SOCK_PATH)+1);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(sock_remote, (struct sockaddr *)&remote, len) == -1) {
		perror("connect");
		USB_LOG("FAIL: connect(sock_remote, (struct sockaddr *)&remote, len)");
		return -1;
	}
	__USB_FUNC_EXIT__ ;
	return sock_remote;
}

/* This function closes socket for ipc with usb-server */
int ipc_noti_server_close(int *sock_remote)
{
	__USB_FUNC_ENTER__ ;
	if (!sock_remote) return -1;
	close (*sock_remote);
	__USB_FUNC_EXIT__ ;
	return 0;
}

/* This function notices something to client app by ipc with socket and gets the results */
int notice_to_client_app(int sock_remote, int request, char *answer)
{
	__USB_FUNC_ENTER__ ;
	int t;
	char str[SOCK_STR_LEN];
	USB_LOG("notice: %d\n", request);
	snprintf(str, SOCK_STR_LEN, "%d", request);
	if (send (sock_remote, str, strlen(str)+1, 0) == -1) {
		USB_LOG("FAIL: send (sock_remote, str, strlen(str)+1, 0)\n");
		return -1;
	}
	if ((t = recv(sock_remote, answer, SOCK_STR_LEN, 0)) > 0) {
		USB_LOG("[CLIENT] Received value: %s\n", answer);
	} else {
		USB_LOG("FAIL: recv(sock_remote, str, SOCK_STR_LEN, 0)\n");
		return -1;
	}
	__USB_FUNC_EXIT__ ;
	return 0;
}
