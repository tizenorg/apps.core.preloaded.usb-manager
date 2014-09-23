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

#include <vconf.h>
#include <devman.h>

/* For multi-user support */
#include <tzplatform_config.h>

#include "um_common.h"

#define SOCK_PATH        "/tmp/usb_server_sock"
#define ACC_SOCK_PATH    "/tmp/usb_acc_sock"

#define USB_SYSPOPUP     "usb-syspopup"
#define SYSPOPUP_TYPE    "_SYSPOPUP_TYPE"

#define ACC_MANUFACTURER "_ACC_MANUFACTURER"
#define ACC_MODEL        "_ACC_MODEL"
#define ACC_DESCRIPTION  "_ACC_DESCRIPTION"
#define ACC_VERSION      "_ACC_VERSION"
#define ACC_URI          "_ACC_URI"
#define ACC_SERIAL       "_ACC_SERIAL"

#define HOST_CLASS       "_HOST_CLASS"
#define HOST_SUBCLASS    "_HOST_SUBCLASS"
#define HOST_PROTOCOL    "_HOST_PROTOCOL"
#define HOST_IDVENDOR    "_HOST_IDVENDOR"
#define HOST_IDPRODUCT   "_HOST_IDPRODUCT"

#define LAUNCH_RETRY 10

#define USER  tzplatform_getuid(TZ_USER_NAME)
#define GROUP tzplatform_getgid(TZ_SYS_USER_GROUP)

int check_usbclient_connection()
{
	__USB_FUNC_ENTER__ ;
	int status = -1;
	int ret = -1;
	ret = vconf_get_int(VCONFKEY_SYSMAN_USB_STATUS, &status);
	if(0 == ret && VCONFKEY_SYSMAN_USB_AVAILABLE == status) {
		USB_LOG("USB connection status: %d", status);
		__USB_FUNC_EXIT__ ;
		return USB_CLIENT_CONNECTED;
	}
	__USB_FUNC_EXIT__ ;
	return USB_CLIENT_DISCONNECTED;
}

int check_usbhost_connection()
{
	__USB_FUNC_ENTER__ ;
	int status = -1;
	int ret = -1;
	ret = vconf_get_int(VCONFKEY_SYSMAN_USB_HOST_STATUS, &status);
	if(0 == ret && VCONFKEY_SYSMAN_USB_HOST_CONNECTED == status) {
		USB_LOG("USB connection status: %d", status);
		__USB_FUNC_EXIT__ ;
		return USB_HOST_CONNECTED;
	}
	__USB_FUNC_EXIT__ ;
	return USB_HOST_DISCONNECTED;
}

static int add_acc_element_to_bundle(bundle *b, void *device)
{
	__USB_FUNC_ENTER__ ;
	assert(b);
	assert(device);

	int ret;
	UsbAccessory *usbAcc = (UsbAccessory *)device;

	if (!(usbAcc->manufacturer) || !(usbAcc->model) || !(usbAcc->description)
		|| !(usbAcc->version) || !(usbAcc->uri) || !(usbAcc->serial)) {
		USB_LOG("ERROR: Input parameters are NULL");
		return -1;
	}

	ret = bundle_add(b, ACC_MANUFACTURER, usbAcc->manufacturer);
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add(%s)", ACC_MANUFACTURER);
		return -1;
	}
	ret = bundle_add(b, ACC_MODEL, usbAcc->model);
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add(%s)", ACC_MODEL);
		return -1;
	}
	ret = bundle_add(b, ACC_DESCRIPTION, usbAcc->description);
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add(%s)", ACC_DESCRIPTION);
		return -1;
	}
	ret = bundle_add(b, ACC_VERSION, usbAcc->version);
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add(%s)", ACC_VERSION);
		return -1;
	}
	ret = bundle_add(b, ACC_URI, usbAcc->uri);
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add(%s)", ACC_URI);
		return -1;
	}
	ret = bundle_add(b, ACC_SERIAL, usbAcc->serial);
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add(%s)", ACC_SERIAL);
		return -1;
	}

	__USB_FUNC_EXIT__ ;
	return 0;
}

static int add_integer_to_bundle(bundle *b, char *key, int value)
{
	__USB_FUNC_ENTER__ ;
	assert(b);
	assert(key);

	int ret;
	char str[DEVICE_ELEMENT_LEN];

	ret = snprintf(str, sizeof(str), "%d", value);
	if (0 > ret) {
		USB_LOG("FAIL: snprintf(): %d", ret);
		return -1;
	}
	ret = bundle_add(b, key, str);
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add(%s)", str);
		return -1;
	}

	__USB_FUNC_EXIT__ ;
	return 0;
}

static int add_host_element_to_bundle(bundle *b, void *device)
{
	__USB_FUNC_ENTER__ ;
	assert(b);
	assert(device);

	int ret;
	UsbInterface *devIf = (UsbInterface *)device;

	ret = add_integer_to_bundle(b, HOST_CLASS, devIf->ifClass);
	if (0 > ret) {
		USB_LOG("FAIL: add_integer_to_bundle(): %d", ret);
		return -1;
	}
	ret = add_integer_to_bundle(b, HOST_SUBCLASS, devIf->ifSubClass);
	if (0 > ret) {
		USB_LOG("FAIL: add_integer_to_bundle(): %d", ret);
		return -1;
	}
	ret = add_integer_to_bundle(b, HOST_PROTOCOL, devIf->ifProtocol);
	if (0 > ret) {
		USB_LOG("FAIL: add_integer_to_bundle(): %d", ret);
		return -1;
	}
	ret = add_integer_to_bundle(b, HOST_IDVENDOR, devIf->ifIdVendor);
	if (0 > ret) {
		USB_LOG("FAIL: add_integer_to_bundle(): %d", ret);
		return -1;
	}
	ret = add_integer_to_bundle(b, HOST_IDPRODUCT, devIf->ifIdProduct);
	if (0 > ret) {
		USB_LOG("FAIL: add_integer_to_bundle(): %d", ret);
		return -1;
	}

	__USB_FUNC_EXIT__ ;
	return 0;
}

int launch_usb_syspopup(UmMainData *ad, POPUP_TYPE popupType, void *device)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);

	// TODO : display a popup

	__USB_FUNC_EXIT__ ;
	return 0;
}

int ipc_request_server_init()
{
	__USB_FUNC_ENTER__ ;
	int sockFd = -1;
	struct sockaddr_un local;

	if ((sockFd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		USB_LOG("FAIL: socket()");
		return -1;
	}
	local.sun_family = AF_UNIX;
	strncpy(local.sun_path, SOCK_PATH, strlen(SOCK_PATH)+1);
	unlink(local.sun_path);

	if (bind(sockFd, (struct sockaddr *)&local, sizeof(local)) == -1) {
		USB_LOG("FAIL: bind()");
		close(sockFd);
		return -1;
	}
	if (0 != chown(SOCK_PATH, USER, GROUP)) {
		USB_LOG("FAIL: chown(%s, %s, %s)", SOCK_PATH, USER, GROUP);
		close(sockFd);
		return -1;
	}
	if (0 != chmod(SOCK_PATH, 0777)) {
		USB_LOG("FAIL: chmod(%s, 0777)", SOCK_PATH);
		close(sockFd);
		return -1;
	}
	if (listen(sockFd, 5) == -1) {
		USB_LOG("FAIL: listen((*sock_local), 5)");
		close(sockFd);
		return -1;
	}
	__USB_FUNC_EXIT__ ;
	return sockFd;
}

/* This function initializes socket for ipc with usb-server */
int ipc_noti_server_init()
{
	__USB_FUNC_ENTER__ ;
	int sockFd;
	struct sockaddr_un remote;
	if ((sockFd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		USB_LOG("FAIL: socket()");
		return -1;
	}
	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, ACC_SOCK_PATH, strlen(SOCK_PATH)+1);

	if (connect(sockFd, (struct sockaddr *)&remote, sizeof(remote)) == -1) {
		USB_LOG("FAIL: connect()");
		close(sockFd);
		return -1;
	}
	__USB_FUNC_EXIT__ ;
	return sockFd;
}

/* This function notices something to client app by ipc with socket and gets the results */
int notice_to_client_app(int sockFd, int request, char *answer, int answerLen)
{
	__USB_FUNC_ENTER__ ;
	assert(answer);
	int t;
	char str[SOCK_STR_LEN];
	USB_LOG("notice: %d\n", request);
	snprintf(str, SOCK_STR_LEN, "%d", request);
	if (send (sockFd, str, strlen(str)+1, 0) == -1) {
		USB_LOG("FAIL: send (sockFd, str, strlen(str)+1, 0)\n");
		return -1;
	}
	if ((t = recv(sockFd, answer, answerLen, 0)) > 0) {
		if (t < answerLen) {
			answer[t] = '\0';
		} else { /* t == answerLen */
			answer[answerLen-1] = '\0';
		}
		USB_LOG("[CLIENT] Received value: %s", answer);
	} else {
		USB_LOG("FAIL: recv(sockFd, str, SOCK_STR_LEN, 0)");
		return -1;
	}
	__USB_FUNC_EXIT__ ;
	return 0;
}

bool is_emul_bin()
{
	__USB_FUNC_ENTER__ ;
	int ret = -1;
	struct utsname name;
	ret = uname(&name);
	if (ret < 0) {
		__USB_FUNC_EXIT__ ;
		return true;
	}

	USB_LOG("Machine name: %s", name.machine);
	if (strcasestr(name.machine, "emul")) {
		__USB_FUNC_EXIT__ ;
		return true;
	}

	__USB_FUNC_EXIT__ ;
	return false;
}

