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

#ifndef __UM_COMMON_H__
#define __UM_COMMON_H__

#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <appcore-common.h>
#include <vconf.h>
#include <devman.h>
#include <sys/types.h>
#include <glib.h>
#include <libusb.h>
#include <sys/utsname.h>
#include <pmapi.h>
#include <assert.h>
#include <libudev.h>
#include <notification.h>
#include "um_data.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_STR_LEN 1542 /* 6 elements + 5 separators + 1 NULL terminator
				= 256 * 6 + 5 * 1 + 1 = 1542 */
#define USB_ICON_PATH PREFIX"/share/usb-server/data/usb_icon.png"

#include <dlog.h>
#undef LOG_TAG
#define LOG_TAG "USB_SERVER"
#define USB_LOG(fmt, args...)         SLOGD(fmt, ##args)
#define USB_LOG_ERROR(fmt, args...)   SLOGE(fmt, ##args)
#define __USB_FUNC_ENTER__            USB_LOG("ENTER")
#define __USB_FUNC_EXIT__             USB_LOG("EXIT")

#define FREE(arg) \
	do { \
		if(arg) { \
			free((void *)arg); \
			arg = NULL; \
		} \
	} while (0);

#ifndef retv_if
#define retv_if(expr, val) \
	do { \
		if(expr) { \
			return (val); \
		} \
	} while (0)
#endif

#define um_retvm_if(expr, val, fmt, arg...) \
	do { \
		if(expr) { \
			USB_LOG_ERROR(fmt, ##arg); \
			return (val); \
		} \
	} while (0);

#define um_retm_if(expr, fmt, arg...) \
	do { \
		if(expr) { \
			USB_LOG_ERROR(fmt, ##arg); \
			return; \
		} \
	} while (0);

int call_cmd(char* cmd);
int check_usbclient_connection();
int check_usbhost_connection();
int check_storage_connection();
int launch_usb_syspopup(UmMainData *ad, POPUP_TYPE _popup_type, void *device);
int notice_to_client_app(int sock_remote, int request, char *answer, int answer_len);

int ipc_request_server_init();
int ipc_request_server_close(UmMainData *ad);
int ipc_noti_server_init();
int ipc_noti_server_close(int *sock_remote);
bool is_emul_bin();

#endif /* __UM_COMMON_H__ */
