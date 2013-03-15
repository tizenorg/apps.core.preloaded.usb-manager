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

#include "um_usb_server.h"
#include <vconf.h>
#include <signal.h>

#define LIBUSB_WAIT 1000000 /* Wait for 1 sec */
#define FAIL_WAIT 100000 /* Wait for 0.1 sec */

char *tempAppId;
int tempVendor, tempProduct;
static struct sigaction sig_pipe_old_act;

static void um_usb_server_release_handler(UmMainData *ad);
static void um_usbhost_vconf_key_ignore(UmMainData *ad);
static void um_usbclient_vconf_key_ignore(UmMainData *ad);
static Eina_Bool answer_to_ipc(void *data, Ecore_Fd_Handler *fd_handler);

static void sig_pipe_handler(int signo, siginfo_t *info, void *data)
{

}

void um_signal_init()
{
	__USB_FUNC_ENTER__;
	struct sigaction sig_act;
	sig_act.sa_handler = NULL;
	sig_act.sa_sigaction = sig_pipe_handler;
	sig_act.sa_flags = SA_SIGINFO;
	sigemptyset(&sig_act.sa_mask);
	sigaction(SIGPIPE, &sig_act, &sig_pipe_old_act);
	__USB_FUNC_EXIT__;
}

static void terminate_usbclient_connection(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int ret;
	int status;

	um_uevent_control_stop(ad);

	um_usb_server_release_handler(ad);

	um_usbclient_vconf_key_ignore(ad);

	disconnectUsbClient(ad);

	/* If USB accessory is removed, the vconf value of accessory status should be updated */
	ret = vconf_get_int(VCONFKEY_USB_ACCESSORY_STATUS, &status);
	if (0 == ret && VCONFKEY_USB_ACCESSORY_STATUS_CONNECTED == status) {
		ret = vconf_set_int(VCONFKEY_USB_ACCESSORY_STATUS,
						VCONFKEY_USB_ACCESSORY_STATUS_DISCONNECTED);
		if(0 != ret)
			USB_LOG("FAIL: vconf_set_int(VCONFKEY_USB_ACCESSORY_STATUS");
		disconnect_accessory(ad);
	}

	FREE(ad->usbAcc);

	ecore_main_loop_quit();

	__USB_FUNC_EXIT__;
}

static void terminate_usbhost_connection(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	um_uevent_control_stop(ad);

	um_usb_server_release_handler(ad);

	um_usbhost_vconf_key_ignore(ad);

	disconnect_usb_host(ad);

	ecore_main_loop_quit();

	__USB_FUNC_EXIT__;
}

static void um_usbclient_chgdet_cb(keynode_t *in_key, void *data)
{
	__USB_FUNC_ENTER__;
	assert(data);

	UmMainData *ad = (UmMainData *)data;
	int status;
	int ret;

	status = check_usbclient_connection();

	switch(status) {
	case USB_CLIENT_DISCONNECTED:
		terminate_usbclient_connection(ad);
		break;

	case USB_CLIENT_CONNECTED:
		ret = connectUsbClient(ad);
		if (0 != ret) {
			USB_LOG("FAIL: connectUsbClient(ad)");
			return;
		}

		if (USB_CLIENT_DISCONNECTED != check_usbclient_connection())
			break;

		terminate_usbclient_connection(ad);
		break;

	default:
		break;
	}
	__USB_FUNC_EXIT__;
}

static void um_usbhost_chgdet_cb(keynode_t *in_key, void *data)
{
	__USB_FUNC_ENTER__;

	UmMainData *ad;
	int status;

	if (!data)
		return;

	ad = (UmMainData *)data;
	status = check_usbhost_connection();

	if (USB_HOST_DISCONNECTED != status)
		return;

	terminate_usbhost_connection(ad);

	__USB_FUNC_EXIT__;
}

static int um_usbclient_vconf_key_notify(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int ret;
	int i;

	i = 0;
	do {
		ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_STATUS,
						um_usbclient_chgdet_cb, ad);
		USB_LOG("vconf_notify_key_changed(): %d", ret);
		if (0 != ret)
			usleep(FAIL_WAIT);
	} while (0 != ret && ++i < 10) ;

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, change_prev_mode_cb, ad);
	if (0 != ret) {
		USB_LOG("FAIL: vconf_notify_key_changed(SETAPPL_USB_SEL_MODE_INT)");
		return -1;
	}

#ifndef SIMULATOR
	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL, debug_mode_cb, ad);
	if (0 != ret) {
		USB_LOG("FAIL: vconf_notify_key_changed(SETAPPL_USB_DEBUG_MODE_BOOL)");
		return -1;
	}
#endif

	ret = vconf_notify_key_changed(VCONFKEY_USB_SEL_MODE, change_mode_cb, ad);
	if (0 != ret) {
		USB_LOG("FAIL: vconf_notify_key_changed(VCONFKEY_USB_SEL_MODE)");
		return -1;
	}

	ret = vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE, change_hotspot_status_cb, ad);
	if (0 != ret)
		USB_LOG("ERROR: vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE)");

	__USB_FUNC_EXIT__;
	return 0;
}

static void um_usbclient_vconf_key_ignore(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, change_prev_mode_cb);
	if (0 != ret)
		USB_LOG("FAIL: vconf_ignore_key_changed(VCONFKEY_SETAPPL_USB_MODE_INT)");

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_STATUS, um_usbclient_chgdet_cb);
	if (0 != ret)
		USB_LOG("FAIL: vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_STATUS)");

	ret = vconf_ignore_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE, change_hotspot_status_cb);
	if (0 != ret)
		USB_LOG("FAIL: vconf_ignore_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE)");

#ifndef SIMULATOR
	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL, debug_mode_cb);
	if (0 != ret)
		USB_LOG("FAIL: vconf_notify_key_changed(VCONFKEY_USB_DEBUG_MODE_BOOL)");
#endif

	ret = vconf_ignore_key_changed(VCONFKEY_USB_SEL_MODE, change_mode_cb);
	if (0 != ret)
		USB_LOG("FAIL: vconf_notify_key_changed(VCONFKEY_USB_SEL_MODE)");

	__USB_FUNC_EXIT__;
}

static int um_usbhost_vconf_key_notify(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int ret;

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS,
						um_usbhost_chgdet_cb, ad);
	if (0 != ret) {
		USB_LOG("FAIL: vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS)");
		return -1;
	}

	__USB_FUNC_EXIT__;
	return 0;
}

static void um_usbhost_vconf_key_ignore(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS,
						um_usbhost_chgdet_cb);
	if (0 != ret)
		USB_LOG("FAIL: vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS)");

	__USB_FUNC_EXIT__;
}

static int um_usb_server_register_handler(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	/* Init server socket for IPC with Accessroy apps and Host apps */
	ad->serverSock = ipc_request_server_init();
	if (0 > ad->serverSock) {
		USB_LOG("FAIL: ipc_request_server_init()");
		return -1;
	}

	/* Add fd handler for pipe between main thread and IPC thread */
	ad->ipcRequestServerFdHandler = ecore_main_fd_handler_add(ad->serverSock,
						ECORE_FD_READ, answer_to_ipc, ad, NULL, NULL);
	if (NULL == ad->ipcRequestServerFdHandler) {
		USB_LOG("FAIL: ecore_main_fd_handler_add()");
		close(ad->serverSock);
		return -1;
	}

	__USB_FUNC_EXIT__;
	return 0;
}

static void um_usb_server_release_handler(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	/* Remove fd handler for pipe between main thread and IPC thread */
	if (ad->ipcRequestServerFdHandler) {
		ecore_main_fd_handler_del(ad->ipcRequestServerFdHandler);
		ad->ipcRequestServerFdHandler = NULL;
	}

	/* Close server socket for IPC with Accessroy apps and Host apps */
	close(ad->serverSock);

	__USB_FUNC_EXIT__;
}

#ifndef SIMULATOR
static void um_usbclient_value_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int ret;
	int debugMode;

	ret = vconf_get_bool(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL, &debugMode);
	if (ret < 0) {
		USB_LOG("FAIL: vconf_get_int(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL)");
		/* Set debug mode to true to find problems */
		debugMode = 1;
		ret = vconf_set_bool(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL, debugMode);
		if (ret < 0)
			USB_LOG("FAIL: vconf_set_bool(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL()");
	}

	if (debugMode == 0) {
		ad->prevDebugMode = false;
		ad->curDebugMode = false;
	} else {
		ad->prevDebugMode = true;
		ad->curDebugMode = true;
	}

	accessory_info_init(ad);
	__USB_FUNC_EXIT__;
}
#else
static void um_usbclient_value_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	accessory_info_init(ad);
	__USB_FUNC_EXIT__;
}
#endif

static void um_usbhost_device_list_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int ret;

	ret = get_default_usb_device(ad);
	if (ret < 0) {
		USB_LOG("FAIL: get_default_usb_device()");
	}

	show_all_usb_devices(ad->defaultDevList);

	ad->devList = NULL;

	__USB_FUNC_EXIT__;
}

static int noti_selected_btn(UmMainData *ad, int input)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int sockFd;
	char buf[SOCK_STR_LEN];
	int ret;
	int ipc_result;
	switch (input) {
	case REQ_ACC_PERM_NOTI_YES_BTN:
		ret = grant_accessory_permission(ad, tempAppId);
		if (0 != ret) {
			USB_LOG("FAIL: grant_accessory_permission(appId)");
		}
		break;
	case REQ_HOST_PERM_NOTI_YES_BTN:
		ret = grant_host_permission(ad, tempAppId, tempVendor, tempProduct);
		if (0 != ret) {
			USB_LOG("grand_host_permission()");
		}
		break;
	default:
		break;
	}

	FREE(tempAppId);
	tempVendor = 0;
	tempProduct = 0;

	sockFd = ipc_noti_server_init();
	um_retvm_if(sockFd < 0, -1, "FAIL: ipc_noti_server_init()");

	ret = notice_to_client_app(sockFd, input, buf, sizeof(buf));
	if (0 != ret) {
		close(sockFd);
		return -1;
	}

	close(sockFd);

	ipc_result = atoi(buf);
	if (IPC_SUCCESS != ipc_result) {
		return -1;
	}

	__USB_FUNC_EXIT__;
	return 0;
}

static int divide_str(char* str1, char** str2)
{
	__USB_FUNC_ENTER__;
	assert(str1);
	assert(str2);

	char *tmp = str1;
	while (*tmp++) {
		if('|' == *tmp) {
			*tmp = '\0';
			tmp++;
			break;
		}
		USB_LOG("#");
	}
	*str2 = tmp;
	__USB_FUNC_EXIT__;
	return 0;
}

static Eina_Bool answer_to_ipc(void *data, Ecore_Fd_Handler *fd_handler)
{
	__USB_FUNC_ENTER__;
	assert(data);

	UmMainData *ad = (UmMainData *)data;
	char str[SOCK_STR_LEN];
	int fd, input, n;
	int sockFd;
	socklen_t t;
	int ret = -1;
	struct sockaddr_un remote;
	char *appId = NULL;
	char *device = NULL;
	char *strVendor = NULL;
	char *strProduct = NULL;
	int vendor = -1;
	int product = -1;

	if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ) == EINA_FALSE)
		return ECORE_CALLBACK_RENEW;
	fd = ecore_main_fd_handler_fd_get(fd_handler);

	t = sizeof(remote);
	if ((sockFd
		= accept(ad->serverSock, (struct sockaddr *)&remote, &t)) == -1) {
		USB_LOG("FAIL: accept(ad->serverSock, (struct sockaddr *)&remote, &t)\n");
		return ECORE_CALLBACK_RENEW;
	}

	n = recv(sockFd, str, sizeof(str), 0);
	if (n <= 0) {
		snprintf(str, sizeof(str), "ERROR");
	} else {
		if (n < sizeof(str)) {
			str[n] = '\0';
		} else { /* n == sizeof(str) */
			str[sizeof(str)-1] = '\0';
		}
		USB_LOG("[SERVER] Received value: %s", str);
		ret = divide_str(str, &appId);
		um_retvm_if(ret < 0, ECORE_CALLBACK_RENEW, "FAIL: divide_str(str, &appId)\n");
		input = atoi(str);
		USB_LOG("input: %d, appId: %s\n", input, appId);
		switch(input) {
		case REQ_HOST_PERMISSION:
		case HAS_HOST_PERMISSION:
		case REQ_HOST_PERM_NOTI_YES_BTN:
		case REQ_HOST_PERM_NOTI_NO_BTN:
		case REQ_HOST_CONNECTION:
			ret = divide_str(appId, &strVendor);
			um_retvm_if(ret < 0, ECORE_CALLBACK_RENEW, "FAIL: divide_str(appId, &strVendor)\n");
			ret = divide_str(strVendor, &strProduct);
			um_retvm_if(ret < 0, ECORE_CALLBACK_RENEW, "FAIL: divide_str(appId, &strProduct)\n");
			vendor = atoi(strVendor);
			product = atoi(strProduct);
			USB_LOG("appId: %s, vendor: %d, product: %d\n", appId, vendor, product);
			break;
		case UNMOUNT_USB_STORAGE:
			device = appId;
			break;
		default:
			break;
		}

		switch(input) {
		case LAUNCH_APP_FOR_ACC:
			ret = grant_accessory_permission(ad, appId);
			if (0 != ret) {
				USB_LOG("FAIL: grant_accessory_permission(appId)");
				snprintf(str, sizeof(str), "%d", IPC_ERROR);
				break;
			}
			ret = launch_acc_app(ad->permAccAppId);
			if (0 != ret) {
				USB_LOG("FAIL: launch_acc_app(appId)");
				snprintf(str, sizeof(str), "%d", IPC_ERROR);
				break;
			}
			snprintf(str, sizeof(str), "%d", IPC_SUCCESS);
			break;
		case REQ_ACC_PERMISSION:
			tempAppId = strdup(appId);
			USB_LOG("tempAppId: %s\n", tempAppId);
			launch_usb_syspopup(ad, REQ_ACC_PERM_POPUP, NULL);
			snprintf(str, sizeof(str), "%d", IPC_SUCCESS);
			break;
		case HAS_ACC_PERMISSION:
			if (true == has_accessory_permission(ad, appId)) {
				snprintf(str, sizeof(str), "%d", IPC_SUCCESS);
			} else {
				snprintf(str, sizeof(str), "%d", IPC_FAIL);
			}
			break;
		case REQ_ACC_PERM_NOTI_YES_BTN:
		case REQ_ACC_PERM_NOTI_NO_BTN:
			snprintf(str, sizeof(str), "%d", IPC_SUCCESS);
			ret = noti_selected_btn(ad, input);
			if (ret < 0) USB_LOG("FAIL: noti_selected_btn(input)\n");
			break;
		case GET_ACC_INFO:
			snprintf(str, sizeof(str), "%s|%s|%s|%s|%s|%s",
							ad->usbAcc->manufacturer,
							ad->usbAcc->model,
							ad->usbAcc->description,
							ad->usbAcc->version,
							ad->usbAcc->uri,
							ad->usbAcc->serial);
			break;
		case ERROR_POPUP_OK_BTN:
			usb_connection_selected_btn(ad, input);
			snprintf(str, sizeof(str), "%d", IPC_SUCCESS);
			break;
		case LAUNCH_APP_FOR_HOST:
			ret = grant_host_permission(ad, appId, vendor, product);
			if (0 != ret) {
				USB_LOG("FAIL: grandHostPermission()\n");
				snprintf(str, sizeof(str), "%d", IPC_ERROR);
				break;
			}
			ret = launch_host_app(appId);
			if (0 != ret) {
				USB_LOG("FAIL: launch_host_app(appId)\n");
				snprintf(str, sizeof(str), "%d", IPC_ERROR);
				break;
			}
			snprintf(str, sizeof(str), "%d", IPC_SUCCESS);
			break;
		case REQ_HOST_PERMISSION:
			tempAppId = strdup(appId);
			tempVendor = vendor;
			tempProduct = product;
			launch_usb_syspopup(ad, REQ_HOST_PERM_POPUP, NULL);
			snprintf(str, sizeof(str), "%d", IPC_SUCCESS);
			break;
		case HAS_HOST_PERMISSION:
			if (EINA_TRUE == has_host_permission(ad, appId, vendor, product)) {
				snprintf(str, sizeof(str), "%d", IPC_SUCCESS);
			} else {
				snprintf(str, sizeof(str), "%d", IPC_FAIL);
			}
			break;
		case REQ_HOST_PERM_NOTI_YES_BTN:
		case REQ_HOST_PERM_NOTI_NO_BTN:
			snprintf(str, sizeof(str), "%d", IPC_SUCCESS);
			ret = noti_selected_btn(ad, input);
			if (ret < 0) USB_LOG("FAIL: noti_selected_btn(input)\n");
			break;
		case REQ_HOST_CONNECTION:
			if (EINA_TRUE == is_host_connected(ad, vendor, product)) {
				snprintf(str, sizeof(str), "%d", IPC_SUCCESS);
			} else {
				snprintf(str, sizeof(str), "%d", IPC_FAIL);
			}
			break;
		case IS_EMUL_BIN:
			if (is_emul_bin()) {
				snprintf(str, sizeof(str), "%d", IPC_SUCCESS);
			} else {
				snprintf(str, sizeof(str), "%d", IPC_FAIL);
			}
			break;
		case UNMOUNT_USB_STORAGE:
			if (!device) {
				snprintf(str, SOCK_STR_LEN, "%d", IPC_ERROR);
				break;
			}
			um_uevent_mass_storage_removed(ad, device);
			snprintf(str, sizeof(str), "%d", IPC_SUCCESS);
			break;
		default:
			snprintf(str, sizeof(str), "%d", IPC_ERROR);
			break;
		}
	}
	USB_LOG("str: %s", str);

	if(send(sockFd, str, strlen(str)+1, 0) < 0) {
		USB_LOG("FAIL: send(sockFd, str, strlen(str)+1, 0)\n");
	}

	__USB_FUNC_EXIT__;
	return ECORE_CALLBACK_RENEW;
}

static int um_usbclient_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int ret;

	ad->isHostOrClient = USB_DEVICE_CLIENT;
	ad->notiAccList = NULL;
	ad->usbAcc = (UsbAccessory*)malloc(sizeof(UsbAccessory));
	if (!(ad->usbAcc)) {
		USB_LOG("FAIL: malloc()");
		return -1;
	}

	um_usbclient_value_init(ad);

	ret = check_driver_version(ad);
	um_retvm_if(0 != ret, -1, "FAIL: check_driver_version(ad)");

	ret = um_usbclient_vconf_key_notify(ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_vconf_key_notify(ad)");

	ret = um_uevent_control_start(ad, USB_DEVICE_CLIENT);
	um_retvm_if(0 != ret, -1, "FAIL: um_uevent_control_start(ad)");


	um_usbclient_chgdet_cb(NULL, ad);

	__USB_FUNC_EXIT__;
	return 0;
}

static int um_usbhost_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int ret;
	int i;

	ad->isHostOrClient = USB_DEVICE_HOST;
	ad->notiHostList = NULL;
	ad->notiMSList = NULL;
	ad->devList = NULL;
	ad->defaultDevList = NULL;
	ad->devMSList = NULL;
	ad->addHostTimer = NULL;
	ad->rmHostTimer = NULL;
	ad->usbctx = NULL;
	ad->tmpfsMounted = false;

	i = 0;
	do {
		ret = libusb_init(&(ad->usbctx));
		USB_LOG("libusb_init() return value : %d", ret);
		if (i > 10) {
			USB_LOG("FAIL: libusb_init()");
			load_connection_popup("Error: Remove and reconnect USB device");
			return 1;
		} else {
			i++;
			USB_LOG("Wait...");
			usleep(LIBUSB_WAIT);
		}
	} while(ret != 0);

	/* Getting default device list */
	um_usbhost_device_list_init(ad);

	ret = um_usbhost_vconf_key_notify(ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_usbhost_vconf_key_notify(ad)");

	ret = um_uevent_control_start(ad, USB_DEVICE_HOST);
	um_retvm_if(0 != ret, -1, "FAIL: um_uevent_control_start(ad)");

	if (USB_HOST_DISCONNECTED == check_usbhost_connection()) {
		um_usbhost_chgdet_cb(NULL, ad);
		return 1;	/* USB host is not available */
	}

	load_connection_popup("IDS_COM_POP_USB_CONNECTOR_CONNECTED");

	/* Check devices which is connected before init process */
	um_uevent_usb_host_added(ad);

	__USB_FUNC_EXIT__;
	return 0;
}

int um_usb_server_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int ret = -1;
	pm_change_state(LCD_NORMAL);

	if (USB_CLIENT_CONNECTED == check_usbclient_connection()) {
		USB_LOG("USB cable is connected");
		ret = um_usbclient_init(ad);
		um_retvm_if(0 > ret, -1, "FAIL: um_usbclient_init(ad)");
	} else {
		if (USB_HOST_CONNECTED == check_usbhost_connection()) {
			USB_LOG("USB host is connected");
			ret = um_usbhost_init(ad);
			um_retvm_if(0 > ret , -1, "FAIL: um_usbhost_init(ad)");
			um_retvm_if(1 == ret, 0, "USB host is not available");
		} else {
			/* USB cable is disconnected */
			USB_LOG("USB cable or device is removed");
			ecore_main_loop_quit();
			__USB_FUNC_EXIT__;
			return 0;
		}
	}

	ret = um_usb_server_register_handler(ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_usb_server_register_handler()");

	__USB_FUNC_EXIT__;
	return 0;
}
