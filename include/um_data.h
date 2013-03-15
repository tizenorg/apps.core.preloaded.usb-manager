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

#ifndef __UM_DATA_H__
#define __UM_DATA_H__

#include <Ecore.h>
#include <unistd.h>
#include <libusb.h>
#define DEVICE_ELEMENT_LEN 256
#define APP_ID_LEN 1024
#define BUF_MAX 256

typedef enum {
	USB_HOST_REF_INTERFACE = 0x0,
	USB_HOST_CDC = 0x2,
	USB_HOST_HID = 0x3,
	USB_HOST_CAMERA = 0x6,
	USB_HOST_PRINTER = 0x7,
	USB_HOST_MASS_STORAGE = 0x8,
	USB_HOST_HUB = 0x9,
	USB_HOST_MISCELLANEOUS = 0xef,
	USB_HOST_VENDOR_SPECIFIC = 0xff
} USB_HOST_CLASS_SUPPORTED;

typedef enum {
	HID_KEYBOARD = 1,
	HID_MOUSE = 2
} HID_PROTOCOL;

typedef enum {
	USB_DEVICE_HOST = 0,
	USB_DEVICE_CLIENT,
	USB_DEVICE_UNKNOWN
} IS_HOST_OR_CLIENT;

typedef enum {
	USB_CLIENT_DISCONNECTED = 0,
	USB_CLIENT_CONNECTED,
	USB_HOST_DISCONNECTED,
	USB_HOST_CONNECTED
} USB_CONNECTION_STATUS;

typedef enum {
	USB_NOTI_ACC_ADDED = 0,
	USB_NOTI_ACC_REMOVED,
	USB_NOTI_HOST_ADDED,
	USB_NOTI_HOST_REMOVED,
	USB_NOTI_MASS_STORAGE_ADDED,
	USB_NOTI_MASS_STORAGE_REMOVED
} USB_NOTIFICATION_TYPE;

typedef enum {
	TICKERNOTI_ORIENTATION_TOP = 0,
	TICKERNOTI_ORIENTATION_BOTTOM
} TICKERNOTI_ORIENTATION;

typedef enum {
	CHANGE_COMPLETE = 0,
	IN_MODE_CHANGE
} MODE_CHANGE;

typedef enum {
	ERROR_POPUP = 0,
	SELECT_PKG_FOR_ACC_POPUP,
	SELECT_PKG_FOR_HOST_POPUP,
	REQ_ACC_PERM_POPUP,
	REQ_HOST_PERM_POPUP,
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
	GET_ACC_INFO,

	/* for Host */
	LAUNCH_APP_FOR_HOST = 40,
	REQ_HOST_PERMISSION,
	HAS_HOST_PERMISSION,
	REQ_HOST_PERM_NOTI_YES_BTN,
	REQ_HOST_PERM_NOTI_NO_BTN,
	REQ_HOST_CONNECTION,

	/* for usb storage */
	UNMOUNT_USB_STORAGE = 60

} REQUEST_TO_USB_MANGER;

typedef enum {
	NOTPERMITTED = 0,
	PERMITTED
} HAS_PERM;

typedef enum {
	USB_DRIVER_0_0 = 0,
	USB_DRIVER_1_0,
	USB_DRIVER_1_1,
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
	ACC_SERIAL,
	HOST_CLASS = 11,
	HOST_SUBCLASS,
	HOST_PROTOCOL,
	HOST_IDVENDOR,
	HOST_IDPRODUCT
} USB_DEVICE_ELEMENT;

typedef struct _UsbAccessory {
	char *manufacturer;
	char *model;
	char *description;
	char *version;
	char *uri;
	char *serial;
} UsbAccessory;

typedef struct _UsbInterface {
	int ifClass;
	int ifSubClass;
	int ifProtocol;
	int ifIdVendor;
	int ifIdProduct;
	int ifBus;
	int ifAddress;
	int ifNumber;
	char *ifIManufacturer;
	char *ifIProduct;
	char *permAppId;
} UsbInterface;

typedef struct _MassStorageDevName {
	char				*devName;
} MSDevName;

typedef struct _UsbNotification {
	int				privId;
	char				*title;
	char				*content;
	char				*iconPath;
	int				applist;
	char				*devName; /* for Mass storage  */
	UsbInterface			*devIf; /* for usb host interface except Mass storage */
	UsbAccessory			*accDev; /* for usb accessory device */

} UsbNotification;

typedef struct _UmMainData {
	Ecore_Fd_Handler		*ipcRequestServerFdHandler;
	int				serverSock;

	UsbAccessory			*usbAcc;
	char				*permAccAppId;
	char				*launchedApp;

	libusb_context			*usbctx;
	GList				*devList;
	GList				*defaultDevList;
	GList				*devMSList;
	Ecore_Timer			*addHostTimer;
	Ecore_Timer			*rmHostTimer;
	bool				tmpfsMounted;

	/* System status */
	USB_DRIVER_VERSION		driverVersion;
	int				isHostOrClient;

	/* USB connection */
#ifndef SIMULATOR
	bool				curDebugMode;
	bool				prevDebugMode;
#endif

	/* Notification */
	GList				*notiAccList; /* Accessory device */
	GList				*notiHostList; /* Host device */
	GList				*notiMSList; /* Mass storage device */

	/* Uevent monitoring */
	Ecore_Fd_Handler		*udevFdHandler;
	struct udev_monitor		*udevMon;
	struct udev			*udev;
	int				udevFd;

} UmMainData;

#endif /* __UM_DATA_H__ */
