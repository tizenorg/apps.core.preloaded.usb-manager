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


#ifndef __UM_DATA_H__
#define __UM_DATA_H__

#include <Ecore.h>
#include <unistd.h>
#define ACC_ELEMENT_LEN 256
#define PKG_NAME_LEN 64

typedef enum {
	CHANGE_COMPLETE = 0,
	IN_MODE_CHANGE
} MODE_CHANGE;

typedef enum {
	SYSPOPUP_TYPE = 0,
	MAX_NUM_SYSPOPUP_PARAM
	/* When we need to deliver other parameters to USB-syspopup
	 * add the types of parameters */
} SYSPOPUP_PARAM;

typedef enum {
	ERROR_POPUP = 0,
	SELECT_PKG_FOR_ACC_POPUP,
	REQ_ACC_PERM_POPUP,
	TEST_POPUP,
	MAX_NUM_SYSPOPUP_TYPE
	/* When we need to add other system popup,
	 * Write here the type of popup */
} POPUP_TYPE;

typedef enum {
	IPC_ERROR = 0,
	IPC_FAIL,
	IPC_SUCCESS
} IPC_SIMPLE_RESULT;

typedef enum {
	/* General */
	ERROR_POPUP_OK_BTN = 0,
	IS_EMUL_BIN,

	/* for Accessory */
	LAUNCH_APP_FOR_ACC = 20,
	REQ_ACC_PERMISSION,
	HAS_ACC_PERMISSION,
	REQ_ACC_PERM_NOTI_YES_BTN,
	REQ_ACC_PERM_NOTI_NO_BTN,
	GET_ACC_INFO
} REQUEST_TO_USB_MANGER;

typedef enum {
	NOTPERMITTED = 0,
	PERMITTED
} HAS_PERM;

typedef enum {
	USB_DRIVER_0_0 = 0,
	USB_DRIVER_1_0,
	MAX_NUM_USB_DRIVER_VERSION
	/* We can add kernel versions here */
} USB_DRIVER_VERSION;

typedef enum {
	ACT_SUCCESS = 0x1000,
	ACT_FAIL
} ACTION_RESULT;

typedef enum {
	ACC_MANUFACTURER = 0,
	ACC_MODEL,
	ACC_DESCRIPTION,
	ACC_VERSION,
	ACC_URI,
	ACC_SERIAL
} ACC_ELEMENT;

typedef struct _UsbAccessory {
	char *manufacturer;
	char *model;
	char *description;
	char *version;
	char *uri;
	char *serial;
} UsbAccessory;

typedef struct _UmMainData {
	Ecore_Fd_Handler		*ipcRequestServerFdHandler;
	int						server_sock_local;
	int						server_sock_remote;

	int 					acc_noti_fd;

	UsbAccessory 			*usbAcc;
	char 					*permittedPkgForAcc;
	char 					*launchedApp;

	/* System status */
	USB_DRIVER_VERSION 		driverVersion;

	/* USB connection */
	int						usbSelMode;
} UmMainData;

#endif /* __UM_DATA_H__ */
