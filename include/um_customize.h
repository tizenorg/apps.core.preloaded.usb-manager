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

#ifndef __UM_CUSTOMIZE_H__
#define __UM_CUSTOMIZE_H__

#include <string.h>
#include <vconf.h>
#include "um_common.h"

#define CMD_DR_START \
	"/usr/bin/start_dr.sh"
#define DATA_ROUTER_PATH \
	"/usr/bin/data-router"
#define KERNEL_SET_PATH \
	"/sys/devices/platform/usb_mode/UsbMenuSel"
#define DRIVER_VERSION_PATH \
	"/sys/class/usb_mode/version"
#define DRIVER_VERSION_1_0 \
	"1.0"
#define DRIVER_VERSION_1_1 \
	"1.1"
#define USB_MODE_ENABLE \
	"/sys/class/usb_mode/usb0/enable"
#define USB_VENDOR_ID \
	"/sys/class/usb_mode/usb0/idVendor"
#define USB_PRODUCT_ID \
	"/sys/class/usb_mode/usb0/idProduct"
#define USB_FUNCTIONS \
	"/sys/class/usb_mode/usb0/functions"
#define USB_FUNCS_FCONF \
	"/sys/class/usb_mode/usb0/funcs_fconf"
#define USB_FUNCS_SCONF \
	"/sys/class/usb_mode/usb0/funcs_sconf"
#define USB_DEVICE_CLASS \
	"/sys/class/usb_mode/usb0/bDeviceClass"
#define USB_DEVICE_SUBCLASS \
	"/sys/class/usb_mode/usb0/bDeviceSubClass"
#define USB_DEVICE_PROTOCOL \
	"/sys/class/usb_mode/usb0/bDeviceProtocol"
#define USB_DIAG_CLIENT \
	"/sys/class/usb_mode/usb0/f_diag/clients"
#define DRIVER_VERSION_BUF_LEN  64
#define FILE_PATH_BUF_SIZE      256
#define KERNEL_SET_BUF_SIZE     3
#define KERNEL_DEFAULT_MODE     0
#define KERNEL_ETHERNET_MODE    4
#define __STR_EMUL 				"emul"
#define __LEN_STR_EMUL			4
#define BIN_INFO_FILE_PATH		"/etc/info.ini"
#define USB_SERVER_MESSAGE_DOMAIN \
	"usb-server"

#define TICKERNOTI_SYSPOPUP \
	"tickernoti-syspopup"

int check_driver_version(UmMainData *ad);
int mode_set_kernel(UmMainData *ad, int mode);
void load_connection_popup(char *msg);
bool is_device_supported(int class);
int get_default_usb_device(UmMainData *ad);

#endif

