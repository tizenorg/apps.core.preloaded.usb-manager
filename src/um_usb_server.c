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

char *tempAppId;
int tempVendor, tempProduct;
static struct sigaction sig_pipe_old_act;

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

int um_heynoti_add(int *fd, char *noti, void (*cb)(void *), UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!fd)   return -1;
	if (!noti) return -1;
	if (!cb)   return -1;
	if (!ad)   return -1;

	*fd = heynoti_init();
	if (-1 == *fd) {
		USB_LOG("FAIL: heynoti_init()");
		return -1;
	} else {
		if (-1 == heynoti_subscribe(*fd, noti, cb, ad)) {
			USB_LOG("FAIL: heynoti_subscribe(fd)");
			return -1;
		} else {
			if (-1 == heynoti_attach_handler(*fd)) {
				USB_LOG("FAIL: heynoti_attach_handler(fd)");
				return -1;
			} else {
				USB_LOG("Success to register heynoti ");
			}
		}
	}
	__USB_FUNC_EXIT__;
	return 0;
}

int um_heynoti_remove(int fd, char *noti, void (*cb)(void *))
{
	__USB_FUNC_ENTER__;
	if (!noti) return -1;
	if (!cb) return -1;
	if (heynoti_unsubscribe(fd, noti, cb) < 0) {
		USB_LOG("ERROR: heynoti_unsubscribe() \n");
	}
	if (heynoti_detach_handler(fd) < 0) {
		USB_LOG("ERROR: heynoti_detach_handler() \n");
	}
	heynoti_close(fd);
	 __USB_FUNC_EXIT__;
	return 0;
}

static int terminate_usbclient_connection(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	int ret = -1;
	int status = -1;

	ret = um_usb_server_release_handler(ad);
	if (0 > ret) USB_LOG("FAIL: um_usb_server_release_handler(ad)\n");

	ret = um_usbclient_vconf_key_ignore(ad);
	if (0 > ret) USB_LOG("FAIL: um_usbclient_vconf_key_ignore(ad)");

	ret = um_usbclient_heynoti_unsubscribe(ad);
	if (0 > ret) USB_LOG("FAIL: um_usbclient_heynoti_unsubscribe(ad)");

	ret = disconnectUsbClient(ad);
	if(0 != ret) USB_LOG("FAIL: disconnectUsbClient(ad)");

	/* If USB accessory is removed, the vconf value of accessory status should be updated */
	ret = vconf_get_int(VCONFKEY_USB_ACCESSORY_STATUS, &status);
	if (0 == ret && VCONFKEY_USB_ACCESSORY_STATUS_CONNECTED == status) {
		ret = vconf_set_int(VCONFKEY_USB_ACCESSORY_STATUS,
						VCONFKEY_USB_ACCESSORY_STATUS_DISCONNECTED);
		if(0 != ret) USB_LOG("FAIL: vconf_set_int(VCONFKEY_USB_ACCESSORY_STATUS");
		ret = disconnectAccessory(ad);
		if(0 != ret) USB_LOG("FAIL: disconnectAccessory(ad)\n");
	}

	ecore_main_loop_quit();

	__USB_FUNC_EXIT__;
	return 0;
}

static int terminate_usbhost_connection(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	int ret = -1;

	ret = um_usb_server_release_handler(ad);
	if (0 > ret) USB_LOG("FAIL: um_usb_server_release_handler(ad)\n");

	ret = um_usbhost_vconf_key_ignore(ad);
	if (0 > ret) USB_LOG("FAIL: um_usbhost_vconf_key_ignore(ad)");

	ret = um_usbhost_heynoti_unsubscribe(ad);
	if (0 > ret) USB_LOG("FAIL: um_usbhost_heynoti_unsubscribe(ad)");

	ret = disconnectUsbHost(ad);
	if (0 != ret) USB_LOG("FAIL: disconnectUsbHost(ad)");

	ecore_main_loop_quit();

	__USB_FUNC_EXIT__;
	return 0;
}

static void um_usbclient_chgdet_cb(keynode_t *in_key, void *data)
{
	__USB_FUNC_ENTER__;
	if (!data) return;
	UmMainData *ad = (UmMainData *)data;
	int status = -1;
	int ret = -1;
	status = check_usbclient_connection();

	switch(status) {
	case USB_CLIENT_DISCONNECTED:
		ret = terminate_usbclient_connection(ad);
		um_retm_if(0 != ret, "FAIL: terminate_usbclient_connection(ad)");
		break;
	case USB_CLIENT_CONNECTED:
		ret = connectUsbClient(ad);
		um_retm_if(0 != ret, "FAIL: connectUsbClient(ad)");
		if (USB_CLIENT_DISCONNECTED == check_usbclient_connection()) {
			ret = terminate_usbclient_connection(ad);
			um_retm_if(0 != ret, "FAIL: terminate_usbclient_connection(ad)\n");
		}
		break;
	default:
		break;
	}
	__USB_FUNC_EXIT__;
}

static void um_usbclient_acc_chgdet_cb(void *data)
{
	__USB_FUNC_ENTER__;
	if (!data) return;
	UmMainData *ad = (UmMainData *)data;
	int status;
	int ret;
	status = check_usbclient_connection();
	if (USB_CLIENT_CONNECTED == status) {
		USB_LOG("ACC_CONNECTED %d", status);
		ret = vconf_get_int(VCONFKEY_USB_ACCESSORY_STATUS, &status);
		um_retm_if(0 != ret, "FAIL: vconf_get_int(VCONFKEY_USB_ACCESSORY_STATUS)\n");
		if (VCONFKEY_USB_ACCESSORY_STATUS_DISCONNECTED == status) {
			ret = connectAccessory(ad);
			um_retm_if(ret != 0, "FAIL: connectAccessory(ad)\n");

			ret = vconf_set_int(VCONFKEY_USB_ACCESSORY_STATUS,
								VCONFKEY_USB_ACCESSORY_STATUS_CONNECTED);
			um_retm_if(ret != 0, "FAIL: vconf_set_int(VCONFKEY_USB_ACCESSORY_STATUS)");
		}
	}
	__USB_FUNC_EXIT__;
}

static void um_usbhost_chgdet_cb(keynode_t *in_key, void *data)
{
	__USB_FUNC_ENTER__;
	if (!data) return;
	UmMainData *ad = (UmMainData *)data;
	int ret = -1;
	int status = check_usbhost_connection();

	if (USB_HOST_DISCONNECTED == status) {
		ret = terminate_usbhost_connection(ad);
		um_retm_if(0 > ret, "FAIL: terminnate_usbhost_connection(ad)");
	}

	__USB_FUNC_EXIT__;
}

int um_usbclient_vconf_key_notify(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, change_mode_cb, ad);
	um_retvm_if(0 != ret, -1, "FAIL: vconf_notify_key_changed(VCONFKEY_SETAPPL_USB_MODE_INT)");

#ifndef SIMULATOR
	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL, debug_mode_cb, ad);
	um_retvm_if(0 != ret, -1, "FAIL: vconf_notify_key_changed(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL)");
#endif

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_STATUS, um_usbclient_chgdet_cb, ad);
	um_retvm_if(0 != ret, -1, "FAIL: vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_STATUS)");

	ret = vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE, change_hotspot_status_cb, ad);
	if (0 != ret) {
		USB_LOG("ERROR: vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE)");
	}

	__USB_FUNC_EXIT__;
	return 0;
}

int um_usbclient_vconf_key_ignore(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, change_mode_cb);
	if (0 != ret) USB_LOG("FAIL: vconf_ignore_key_changed(VCONFKEY_SETAPPL_USB_MODE_INT)");

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_STATUS, um_usbclient_chgdet_cb);
	if (0 != ret) USB_LOG("FAIL: vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_STATUS)");

	ret = vconf_ignore_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE, change_hotspot_status_cb);
	if (0 != ret) USB_LOG("FAIL: vconf_ignore_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE)");

#ifndef SIMULATOR
	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL, debug_mode_cb);
	if (0 != ret) USB_LOG("FAIL: vconf_notify_key_changed(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL)");
#endif

	__USB_FUNC_EXIT__;
	return 0;
}

int um_usbhost_vconf_key_notify(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS,
									um_usbhost_chgdet_cb, ad);
	um_retvm_if(0 != ret, -1, "FAIL: vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS)");

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_ADDED_STORAGE_UEVENT,
									add_mass_storage_cb, ad);
	um_retvm_if(0 != ret, -1, "FAIL: vconf_notify_key_changed(VCONFKEY_SYSMAN_ADD_STORAGE_UEVENT)");

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_REMOVED_STORAGE_UEVENT,
									remove_mass_storage_cb, ad);
	um_retvm_if(0 != ret, -1, "FAIL: vconf_notify_key_changed(VCONFKEY_SYSMAN_REMOVE_STORAGE_UEVENT)");

	__USB_FUNC_EXIT__;
	return 0;
}

int um_usbhost_vconf_key_ignore(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS,
									um_usbhost_chgdet_cb);
	if (0 != ret) USB_LOG("FAIL: vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS)");

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_ADDED_STORAGE_UEVENT,
									add_mass_storage_cb);
	if (0 != ret) USB_LOG("FAIL: vconf_ignore_key_changed(VCONFKEY_SYSMAN_ADD_STORAGE_UEVENT)");

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_REMOVED_STORAGE_UEVENT,
									remove_mass_storage_cb);
	if (0 != ret) USB_LOG("FAIL: vconf_ignore_key_changed(VCONFKEY_SYSMAN_REMOVE_STORAGE_UEVENT)");

	__USB_FUNC_EXIT__;
	return 0;
}

int um_usbclient_heynoti_subscribe(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;

	ret = um_heynoti_add(&(ad->acc_noti_fd), "device_usb_accessory",
											um_usbclient_acc_chgdet_cb, ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_heynoti_add(ad->acc_noti_fd)");

	__USB_FUNC_EXIT__;
	return 0;
}

int um_usbclient_heynoti_unsubscribe(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;

	ret = um_heynoti_remove(ad->acc_noti_fd, "device_usb_accessory",
											um_usbclient_acc_chgdet_cb);
	if (0 != ret) USB_LOG("FAIL: um_heynoti_remove(ad->acc_noti_fd)");

	__USB_FUNC_EXIT__;
	return 0;
}

int um_usbhost_heynoti_subscribe(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;

	ret = um_heynoti_add(&(ad->host_add_noti_fd), "device_usb_host_add",
											add_host_noti_cb, ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_heynoti_add(ad->host_add_noti_fd)");

	ret = um_heynoti_add(&(ad->host_remove_noti_fd), "device_usb_host_remove",
											remove_host_noti_cb, ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_heynoti_add(ad->host_remove_noti_fd)");

	__USB_FUNC_EXIT__;
	return 0;
}

int um_usbhost_heynoti_unsubscribe(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;

	ret = um_heynoti_remove(ad->host_add_noti_fd, "device_usb_host_add",
											add_host_noti_cb);
	if (0 != ret) USB_LOG("FAIL: um_heynoti_remove(ad->host_add_noti_fd)");

	ret = um_heynoti_remove(ad->host_remove_noti_fd, "device_usb_host_remove",
											remove_host_noti_cb);
	if (0 != ret) USB_LOG("FAIL: um_heynoti_remove(ad->host_remove_noti_fd)");

	__USB_FUNC_EXIT__;
	return 0;
}

int um_usb_server_register_handler(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;

	/* Init server socket for IPC with Accessroy apps and Host apps */
	ad->server_sock_local = ipc_request_server_init();
	um_retvm_if(0 > ad->server_sock_local, -1, "FAIL: ipc_request_server_init()\n");

	/* Add fd handler for pipe between main thread and IPC thread */
	ad->ipcRequestServerFdHandler = ecore_main_fd_handler_add(ad->server_sock_local,
										ECORE_FD_READ, answer_to_ipc, ad, NULL, NULL);
	um_retvm_if(NULL == ad->ipcRequestServerFdHandler, -1, "FAIL: ecore_main_fd_handler_add()");

	__USB_FUNC_EXIT__;
	return 0;
}

int um_usb_server_release_handler(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;

	/* Remove fd handler for pipe between main thread and IPC thread */
	if (ad->ipcRequestServerFdHandler != NULL) {
		ecore_main_fd_handler_del(ad->ipcRequestServerFdHandler);
		ad->ipcRequestServerFdHandler = NULL;
	}

	/* Close server socket for IPC with Accessroy apps and Host apps */
	ipc_request_server_close(ad);

	__USB_FUNC_EXIT__;
	return 0;
}

#ifndef SIMULATOR
int um_usbclient_value_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;
	int debugMode = 0;

	ret = vconf_get_bool(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL, &debugMode);
	if (ret < 0) {
		USB_LOG("FAIL: vconf_get_int(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL)");
		/* Set debug mode to true to find problems */
		debugMode = 1;
		ret = vconf_set_bool(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL, debugMode);
		if (ret < 0) USB_LOG("FAIL: vconf_set_bool(VCONFKEY_SETAPPL_USB_DEBUG_MODE_BOOL()");
	}
	if (debugMode == 1) {
		ad->prevDebugMode = true;
		ad->curDebugMode = true;
	} else if (debugMode == 0) {
		ad->prevDebugMode = false;
		ad->curDebugMode = false;
	} else {
		USB_LOG("debugMode %d is improper");
		return -1;
	}

	umAccInfoInit(ad);
	__USB_FUNC_EXIT__;
	return 0;
}
#else
int um_usbclient_value_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	umAccInfoInit(ad);
	__USB_FUNC_EXIT__;
	return 0;
}
#endif

int um_usbhost_device_list_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;
	int status;
	int defNumDevices;
	ret = get_default_usb_device(ad);
	if (ret < 0) {
		USB_LOG("FAIL: get_default_usb_device()");
	}

	show_all_usb_devices(ad->defaultDevList, 1);

	ad->devList = NULL;

	__USB_FUNC_EXIT__;
	return 0;
}

int noti_selected_btn(UmMainData *ad, int input)
{
	__USB_FUNC_ENTER__;
	int sock_remote = -1;
	char buf[SOCK_STR_LEN];
	int ret = -1;
	int ipc_result = -1;
	switch (input) {
	case REQ_ACC_PERM_NOTI_YES_BTN:
		ret = grantAccessoryPermission(ad, tempAppId);
		if (0 != ret) {
			USB_LOG("FAIL: grant_permission_to_app(appId)");
		}
		break;
	case REQ_HOST_PERM_NOTI_YES_BTN:
		ret = grantHostPermission(ad, tempAppId, tempVendor, tempProduct);
		if (0 != ret) {
			USB_LOG("grandHostPermission()\n");
		}
		break;
	default:
		break;
	}

	FREE(tempAppId);
	tempVendor = 0;
	tempProduct = 0;
	sock_remote = ipc_noti_server_init();
	um_retvm_if(sock_remote < 0, -1, "FAIL: ipc_noti_server_init()\n");
	ret = notice_to_client_app(sock_remote, input, buf);
	if (ret == 0) {
		ipc_result = atoi(buf);
	}
	ret = ipc_noti_server_close(&sock_remote);
	um_retvm_if(ret < 0, -1, "FAIL: ipc_socket_client_close(&sock_remote)\n");
	if (IPC_SUCCESS != ipc_result) {
		return -1;
	}
	__USB_FUNC_EXIT__;
	return 0;
}

static int divide_str(char* str1, char** str2)
{
	__USB_FUNC_ENTER__;
	if(!str1 || !str2) return -1;
	char *tmp = str1;
	if (str1) {
		while (*tmp++) {
			if('|' == *tmp) {
				*tmp = '\0';
				tmp++;
				break;
			}
			USB_LOG("#");
		}
	}
	*str2 = tmp;
	__USB_FUNC_EXIT__;
	return 0;
}

Eina_Bool answer_to_ipc(void *data, Ecore_Fd_Handler *fd_handler)
{
	__USB_FUNC_ENTER__;
	if (!data) return ECORE_CALLBACK_RENEW;
	UmMainData *ad = (UmMainData *)data;
	char str[SOCK_STR_LEN];
	int fd, input, output, t, n;
	int ret = -1;
	struct sockaddr_un remote;
	char *appId = NULL;
	char *strVendor = NULL;
	char *strProduct = NULL;
	int vendor = -1;
	int product = -1;

	if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ) == EINA_FALSE)
		return ECORE_CALLBACK_RENEW;
	fd = ecore_main_fd_handler_fd_get(fd_handler);

	t = sizeof(remote);
	if ((ad->server_sock_remote
		= accept(ad->server_sock_local, (struct sockaddr *)&remote, &t)) == -1) {
		perror("accept");
		USB_LOG("FAIL: accept(ad->server_sock_local, (struct sockaddr *)&remote, &t)\n");
		return ECORE_CALLBACK_RENEW;
	}
	n = recv(ad->server_sock_remote, str, SOCK_STR_LEN, 0);
	if (n <= 0) {
		snprintf(str, SOCK_STR_LEN, "ERROR");
	} else {
		if (n < SOCK_STR_LEN) {
			str[n] = '\0';
		} else { /* n == SOCK_STR_LEN */
			str[SOCK_STR_LEN-1] = '\0';
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
		default:
			break;
		}

		switch(input) {
		case LAUNCH_APP_FOR_ACC:
			ret = grantAccessoryPermission(ad, appId);
			if (0 != ret) {
				USB_LOG("FAIL: grant_permission_to_app(appId)");
				snprintf(str, SOCK_STR_LEN, "%d", IPC_ERROR);
				break;
			}
			ret = launch_acc_app(ad->permittedPkgForAcc);
			if (0 != ret) {
				USB_LOG("FAIL: launch_app(appId)");
				snprintf(str, SOCK_STR_LEN, "%d", IPC_ERROR);
				break;
			}
			snprintf(str, SOCK_STR_LEN, "%d", IPC_SUCCESS);
			break;
		case REQ_ACC_PERMISSION:
			tempAppId = strdup(appId);
			USB_LOG("tempAppId: %s\n", tempAppId);
			load_system_popup(ad, REQ_ACC_PERM_POPUP);
			snprintf(str, SOCK_STR_LEN, "%d", IPC_SUCCESS);
			break;
		case HAS_ACC_PERMISSION:
			if (EINA_TRUE == hasAccPermission(ad, appId)) {
				snprintf(str, SOCK_STR_LEN, "%d", IPC_SUCCESS);
			} else {
				snprintf(str, SOCK_STR_LEN, "%d", IPC_FAIL);
			}
			break;
		case REQ_ACC_PERM_NOTI_YES_BTN:
		case REQ_ACC_PERM_NOTI_NO_BTN:
			snprintf(str, SOCK_STR_LEN, "%d", IPC_SUCCESS);
			ret = noti_selected_btn(ad, input);
			if (ret < 0) USB_LOG("FAIL: noti_selected_btn(input)\n");
			break;
		case GET_ACC_INFO:
			snprintf(str, SOCK_STR_LEN, "%s|%s|%s|%s|%s|%s",
							ad->usbAcc->manufacturer,
							ad->usbAcc->model,
							ad->usbAcc->description,
							ad->usbAcc->version,
							ad->usbAcc->uri,
							ad->usbAcc->serial);
			break;
		case ERROR_POPUP_OK_BTN:
			usb_connection_selected_btn(ad, input);
			snprintf(str, SOCK_STR_LEN, "%d", IPC_SUCCESS);
			break;
		case LAUNCH_APP_FOR_HOST:
			ret = grantHostPermission(ad, appId, vendor, product);
			if (0 != ret) {
				USB_LOG("FAIL: grandHostPermission()\n");
				snprintf(str, SOCK_STR_LEN, "%d", IPC_ERROR);
				break;
			}
			ret = launch_host_app(appId);
			if (0 != ret) {
				USB_LOG("FAIL: launch_host_app(appId)\n");
				snprintf(str, SOCK_STR_LEN, "%d", IPC_ERROR);
				break;
			}
			snprintf(str, SOCK_STR_LEN, "%d", IPC_SUCCESS);
			break;
		case REQ_HOST_PERMISSION:
			tempAppId = strdup(appId);
			tempVendor = vendor;
			tempProduct = product;
			load_system_popup(ad, REQ_HOST_PERM_POPUP);
			snprintf(str, SOCK_STR_LEN, "%d", IPC_SUCCESS);
			break;
		case HAS_HOST_PERMISSION:
			if (EINA_TRUE == hasHostPermission(ad, appId, vendor, product)) {
				snprintf(str, SOCK_STR_LEN, "%d", IPC_SUCCESS);
			} else {
				snprintf(str, SOCK_STR_LEN, "%d", IPC_FAIL);
			}
			break;
		case REQ_HOST_PERM_NOTI_YES_BTN:
		case REQ_HOST_PERM_NOTI_NO_BTN:
			snprintf(str, SOCK_STR_LEN, "%d", IPC_SUCCESS);
			ret = noti_selected_btn(ad, input);
			if (ret < 0) USB_LOG("FAIL: noti_selected_btn(input)\n");
			break;
		case REQ_HOST_CONNECTION:
			if (EINA_TRUE == is_host_connected(ad, vendor, product)) {
				snprintf(str, SOCK_STR_LEN, "%d", IPC_SUCCESS);
			} else {
				snprintf(str, SOCK_STR_LEN, "%d", IPC_FAIL);
			}
			break;
		case IS_EMUL_BIN:
			if (is_emul_bin()) {
				snprintf(str, SOCK_STR_LEN, "%d", IPC_SUCCESS);
			} else {
				snprintf(str, SOCK_STR_LEN, "%d", IPC_FAIL);
			}
			break;
		default:
			snprintf(str, SOCK_STR_LEN, "%d", IPC_ERROR);
			break;
		}
	}
	USB_LOG("str: %s", str);

	if(send(ad->server_sock_remote, str, strlen(str)+1, 0) < 0) {
		USB_LOG("FAIL: end(ad->server_sock_remote, str, strlen(str)+1, 0)\n");
	}

	__USB_FUNC_EXIT__;
	return ECORE_CALLBACK_RENEW;
}

static int um_usbclient_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	int ret = -1;

	ad->isHostOrClient = USB_DEVICE_CLIENT;

	ret = um_usbclient_value_init(ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_usbclient_value_init(ad)");

	ret = check_driver_version(ad);
	um_retvm_if(0 != ret, -1, "FAIL: check_driver_version(ad)");

	ret = um_usbclient_vconf_key_notify(ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_vconf_key_notify(ad)");

	ret = um_usbclient_heynoti_subscribe(ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_usbclient_heynoti_subscribe(ad)");

	um_usbclient_chgdet_cb(NULL, ad);

	__USB_FUNC_EXIT__;
	return 0;
}

static int um_usbhost_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	int ret = -1;

	ad->isHostOrClient = USB_DEVICE_HOST;
	ad->usbctx = NULL;
	int i = 0;
	do {
		ret = libusb_init(&(ad->usbctx));
		USB_LOG("libusb_init() return value : %d", ret);
		if (i > 10) {
			USB_LOG("FAIL: libusb_init()");
			load_connection_popup(ad, "Error: Remove and reconnect USB device",
										TICKERNOTI_ORIENTATION_TOP);
			return 1;
		} else {
			i++;
			USB_LOG("Wait...");
			sleep(1);
		}
	} while(ret != 0);

	/* Getting default device list */
	ret = um_usbhost_device_list_init(ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_usbhost_device_list_init(ad)");

	ret = um_usbhost_vconf_key_notify(ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_usbhost_vconf_key_notify(ad)");

	ret = um_usbhost_heynoti_subscribe(ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_usbclient_heynoti_subscribe(ad)");

	if (USB_HOST_DISCONNECTED == check_usbhost_connection()) {
		um_usbhost_chgdet_cb(NULL, ad);
		return 1;	/* USB host is not available */
	} else {
		load_connection_popup(ad, "IDS_COM_POP_USB_CONNECTOR_CONNECTED", TICKERNOTI_ORIENTATION_TOP);
	}

	/* Check devices which is connected before init process */
	ret = add_usb_device_to_list(ad);
	um_retvm_if(0 != ret, -1, "FAIL: add_usb_device_to_list(ad)");


	__USB_FUNC_EXIT__;
	return 0;
}

int um_usb_server_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	/* In case that heynoties are subscribed after a noti arrives, 
	 * check whether or not a noti already arrived and do something */
	if(!ad) return -1;
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
