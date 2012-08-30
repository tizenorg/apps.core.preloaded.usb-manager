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


#ifndef __UM_COMMON_H__
#define __UM_COMMON_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <appcore-common.h>
#include <heynoti.h>
#include <vconf.h>
#include <devman.h>
#include <sys/types.h>
#include <glib.h>
#include <syspopup_caller.h>
#include <sys/utsname.h>
#include "um_data.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/usb_server_sock"
#define ACC_SOCK_PATH "/tmp/usb_acc_sock"
#define USB_ACCESSORY_NODE "/dev/usb_accessory"
#define SETTING_USB_ACCESSORY_MODE 5
#define SOCK_STR_LEN 1542 /* 6 elements + 5 separators + 1 NULL terminator
							= 256 * 6 + 5 * 1 + 1 = 1542 */
#define ACC_INFO_NUM 6
#define SETTING_USB_DEFAULT_MODE 0


#define PACKAGE "usb-server" /* for i18n */
#define LOCALEDIR PREFIX"/share/locale"
#define SYSPOPUP_PARAM_LEN 3
#define USB_SYSPOPUP "usb-syspopup"

#include <dlog.h>

#define USB_TAG "USB_SERVER"

#define USB_LOG(format, args...) \
	LOG(LOG_VERBOSE, USB_TAG, "[%s][Ln: %d] " format, (char*)(strrchr(__FILE__, '/')+1), __LINE__, ##args)

#define USB_LOG_ERROR(format, args...) \
	LOG(LOG_ERROR, USB_TAG, "[%s][Ln: %d] " format, (char*)(strrchr(__FILE__, '/')+1), __LINE__, ##args)

#define __USB_FUNC_ENTER__ \
			USB_LOG("Entering: %s()\n", __func__)

#define __USB_FUNC_EXIT__ \
			USB_LOG("Exit: %s()\n", __func__)

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

int check_usb_connection();
int check_storage_connection();
int launch_usb_syspopup(UmMainData *ad, POPUP_TYPE _popup_type);
void load_system_popup(UmMainData *ad, POPUP_TYPE _popup_type);

int ipc_request_server_init();
int ipc_request_server_close(UmMainData *ad);
int ipc_noti_server_init();
int ipc_noti_server_close(int *sock_remote);
bool is_emul_bin();

#endif /* __UM_COMMON_H__ */
