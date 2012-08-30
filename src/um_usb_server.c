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
#include "um_usb_server.h"
#include <vconf.h>
#include <signal.h>

char *tempAppId;
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
				USB_LOG("Success to register heynoti");
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

static int terminate_usb_connection(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	int ret = -1;
	int status = -1;

	ret = um_usb_server_release_handler(ad);
	if (ret < 0) USB_LOG("FAIL: um_usb_server_release_handler(ad)\n");

	ret = disconnectUsb(ad);
	if(0 != ret) USB_LOG("FAIL: disconnectUsb(ad)");

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

static void usb_chgdet_cb(keynode_t *in_key, void *data)
{
	__USB_FUNC_ENTER__;
	if (!data) return;
	UmMainData *ad = (UmMainData *)data;
	int status = -1;
	int ret = -1;
	status = check_usb_connection();
	switch(status) {
	case VCONFKEY_SYSMAN_USB_DISCONNECTED:
		ret = terminate_usb_connection(ad);
		um_retm_if(0 != ret, "FAIL: terminate_usb_connection(ad)");
		break;
	case VCONFKEY_SYSMAN_USB_AVAILABLE:
		ret = connectUsb(ad);
		um_retm_if(0 != ret, "FAIL: connectUsb(ad)");
		if (VCONFKEY_SYSMAN_USB_AVAILABLE != check_usb_connection()) {
			ret = terminate_usb_connection(ad);
			um_retm_if(0 != ret, "FAIL: terminate_usb_connection(ad)\n");
		}
		break;
	default:
		break;
	}
	__USB_FUNC_EXIT__;
}

static void acc_chgdet_cb(void *data)
{
	__USB_FUNC_ENTER__;
	if (!data) return;
	UmMainData *ad = (UmMainData *)data;
	static int status;
	int ret;
	status = check_usb_connection();
	switch(status) {
	case VCONFKEY_SYSMAN_USB_DISCONNECTED:
		USB_LOG("ACC_DISCONNECTED %d", status);
		ret = vconf_get_int(VCONFKEY_USB_ACCESSORY_STATUS, &status);
		um_retm_if(0 != ret, "FAIL: vconf_get_int(VCONFKEY_USB_SERVER_ACCESSORY_STATUS_INT)\n");
		if (VCONFKEY_USB_ACCESSORY_STATUS_CONNECTED == status) {
			ret = vconf_set_int(VCONFKEY_USB_ACCESSORY_STATUS,
						VCONFKEY_USB_ACCESSORY_STATUS_DISCONNECTED);
			um_retm_if(ret != 0, "FAIL: vconf_set_int(VCONFKEY_USB_ACCESSORY_STATUS)");
			ret = disconnectAccessory(ad);
			um_retm_if(ret != 0, "FAIL: disconnectAccessory(ad)\n");
		}
		break;
	case VCONFKEY_SYSMAN_USB_AVAILABLE:
		USB_LOG("ACC_CONNECTED %d", status);
		ret = vconf_get_int(VCONFKEY_USB_ACCESSORY_STATUS, &status);
		um_retm_if(0 != ret, "FAIL: vconf_get_int(VCONFKEY_USB_SERVER_ACCESSORY_STATUS_INT)\n");
		if (VCONFKEY_USB_ACCESSORY_STATUS_DISCONNECTED == status) {
			ret = vconf_set_int(VCONFKEY_USB_ACCESSORY_STATUS,
						VCONFKEY_USB_ACCESSORY_STATUS_CONNECTED);
			um_retm_if(ret != 0, "FAIL: vconf_set_int(VCONFKEY_USB_ACCESSORY_STATUS, CONNECTED)");
			ret = connectAccessory(ad);
			um_retm_if(ret != 0, "FAIL: connectAccessory(ad)\n");
		}
		break;
	default:
		break;
	}
	__USB_FUNC_EXIT__;
}

int um_vconf_key_notify(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;

	/* USB Connection Manager */
	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_STATUS, usb_chgdet_cb, ad);
	um_retvm_if(0 != ret, -1, "FAIL: vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_STATUS)");
	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, change_mode_cb, ad);
	um_retvm_if(0 != ret, -1, "FAIL: vconf_notify_key_changed(VCONFKEY_SETAPPL_USB_MODE_INT)");
	ret = vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE, change_hotspot_status_cb, ad);
	if (0 != ret) {
		USB_LOG("ERROR: vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE)");
	}

	__USB_FUNC_EXIT__;
	return 0;
}

int um_value_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;

	/* USB Connection Manager */
	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, &(ad->usbSelMode));
	um_retvm_if (0 != ret, -1, "FAIL: vconf_get_int(VCONFKEY_SETAPPL_USB_SEL_MODE_INT)\n");
	umAccInfoInit(ad);


	__USB_FUNC_EXIT__;
	return 0;
}

int noti_selected_btn(UmMainData *ad, int input)
{
	__USB_FUNC_ENTER__;
	int sock_remote;
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
	default:
		break;
	}
	FREE(tempAppId);
	sock_remote = ipc_noti_server_init();
	um_retvm_if(sock_remote < 0, -1, "FAIL: ipc_noti_server_init()\n");
	ret = notice_to_client_app(sock_remote, input, buf);
	um_retvm_if(ret < 0, -1, "FAIL: notice_to_client_app(socke_remote, input, buf)\n");
	ret = ipc_noti_server_close(&sock_remote);
	um_retvm_if(ret < 0, -1, "FAIL: ipc_socket_client_close(&sock_remote)\n");
	ipc_result = atoi(buf);
	if (IPC_SUCCESS != ipc_result) {
		return -1;
	}
	__USB_FUNC_EXIT__;
	return 0;
}

Eina_Bool answer_to_ipc(void *data, Ecore_Fd_Handler *fd_handler)
{
	__USB_FUNC_ENTER__;
	if (!data) return ECORE_CALLBACK_CANCEL;
	UmMainData *ad = (UmMainData *)data;
	char str[SOCK_STR_LEN];
	int fd, input, output, t, n;
	int ret = -1;
	struct sockaddr_un remote;

	if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ) == EINA_FALSE)
		return ECORE_CALLBACK_CANCEL;
	fd = ecore_main_fd_handler_fd_get(fd_handler);

	t = sizeof(remote);
	if ((ad->server_sock_remote
			= accept(ad->server_sock_local, (struct sockaddr *)&remote, &t)) == -1) {
		perror("accept");
		USB_LOG("FAIL: accept(ad->server_sock_local, (struct sockaddr *)&remote, &t)\n");
		return ;
	}
	n = recv(ad->server_sock_remote, str, SOCK_STR_LEN, 0);
	if (n <= 0) {
		snprintf(str, SOCK_STR_LEN, "ERROR");
	} else {
		USB_LOG("[SERVER] Received value: %s", str);
		char *appId = str;
		if (str) {
			while (*appId++) {
				if('|' == *appId) {
					*appId = '\0';
					appId++;
					break;
				}
				USB_LOG("#");
			}
		}
		USB_LOG("input: %s, appId: %s\n", str, appId);

		input = atoi(str);
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

int um_usb_server_init(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	um_value_init(ad);
	int ret = -1;

	ret = check_driver_version(ad);
	um_retvm_if(0 != ret, -1, "FAIL: check_driver_version(ad)");

	ad->server_sock_local = ipc_request_server_init();
	um_retvm_if(0 > ad->server_sock_local, -1, "FAIL: ipc_request_server_init()\n");

	ad->ipcRequestServerFdHandler = ecore_main_fd_handler_add(ad->server_sock_local,
							ECORE_FD_READ, answer_to_ipc, ad, NULL, NULL);
	um_retvm_if(NULL == ad->ipcRequestServerFdHandler, -1, "FAIL: ecore_main_fd_handler_add()");

	ret = um_vconf_key_notify(ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_vconf_key_notify(ad)");

	ret = um_heynoti_add(&(ad->acc_noti_fd), "device_usb_accessory", acc_chgdet_cb, ad);
	um_retvm_if(0 != ret, -1, "FAIL: um_heynoti_add(ad->acc_noti_fd)");

	usb_chgdet_cb(NULL, ad);

	__USB_FUNC_EXIT__;
	return 0;
}

int um_usb_server_release_handler(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	int ret = -1;

	if (ad->ipcRequestServerFdHandler != NULL) {
		ecore_main_fd_handler_del(ad->ipcRequestServerFdHandler);
		ad->ipcRequestServerFdHandler == NULL;
	}

	ipc_request_server_close(ad);

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_STATUS, usb_chgdet_cb);
	if (0 != ret) USB_LOG("FAIL: vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_STATUS)");

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_USB_SEL_MODE_INT, change_mode_cb);
	if (0 != ret) USB_LOG("FAIL: vconf_notify_key_changed(VCONFKEY_SETAPPL_USB_MODE_INT)");

	ret = vconf_ignore_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE, change_hotspot_status_cb);
	if (0 != ret) USB_LOG("ERROR: vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE)");

	ret = um_heynoti_remove(ad->acc_noti_fd, "device_usb_accessory", acc_chgdet_cb);
	if (0 != ret) USB_LOG("FAIL: um_heynoti_remove(ad->acc_noti_fd)\n");

	__USB_FUNC_EXIT__;
	return 0;
}
