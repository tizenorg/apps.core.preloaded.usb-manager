/*
 * USB server
 *
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd All Rights Reserved
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

#include "um_customize.h"
#include <iniparser.h>

#define USB_DEFAULT_MODE_CONNECTED "Default USB mode enabled"
#define USB_ACCESSORY_MODE_ENABLED "USB accessory mode enabled"

/* If other kernel versions are added, we should modify this function */
int check_driver_version(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;

	if(!ad) return -1;
	char buffer[DRIVER_VERSION_BUF_LEN];

	FILE *fp = NULL;

	if (access(DRIVER_VERSION_PATH, F_OK) != 0) {
		USB_LOG("This kernel is for C210\n");
		ad->driverVersion = USB_DRIVER_0_0;
	} else {
		fp = fopen(DRIVER_VERSION_PATH, "r");
		um_retvm_if(fp == NULL, -1, "FAIL: fopen(%s)\n", DRIVER_VERSION_PATH);

		if (fgets(buffer, DRIVER_VERSION_BUF_LEN, fp) == NULL) {
			USB_LOG("FAIL: fgets(%s)\n", DRIVER_VERSION_PATH);
			if (fclose(fp) != 0) {
				USB_LOG("FAIL: fclose(fp)\n");
			}
			return -1;
		}

		if (fclose(fp) != 0) {
			USB_LOG("FAIL: fclose(fp)\n");
			return -1;
		}

		if (strncmp(buffer, DRIVER_VERSION_1_0, strlen(DRIVER_VERSION_1_0)) == 0 ) {
			USB_LOG("The driver version is 1.0 \n");
			ad->driverVersion = USB_DRIVER_1_0;
		} else {
			USB_LOG("This kernel version is unknown\n");
			return -1;
		}
	}
	__USB_FUNC_EXIT__ ;
	return 0;
}

int mode_set_kernel(USB_DRIVER_VERSION _version, int mode)
{
	__USB_FUNC_ENTER__ ;

	int ret = -1;
	switch(_version) {
	case USB_DRIVER_0_0:
		USB_LOG("This target is C210 \n");
		ret = mode_set_driver_0_0(mode);
		um_retvm_if (0 != ret, -1, "FAIL: mode_set_driver_0_0(mode)\n");
		break;
	case USB_DRIVER_1_0:
		USB_LOG("This target uses a concept which USB mode can be enabled/disabled\n");
		ret = mode_set_driver_1_0(mode);
		um_retvm_if (0 != ret, -1, "FAIL: mode_set_driver_1_0(mode)\n");
		break;

	/* If other kernel versions are added, add functions here that notice USB mode to the kernel */

	default:
		USB_LOG("This version of kernel is not registered to usb-server.\n");
		__USB_FUNC_EXIT__ ;
		return -1;
	}
	__USB_FUNC_EXIT__ ;
	return 0;
}

static int mode_set_driver_0_0(int mode)
{
	__USB_FUNC_ENTER__ ;

	char buf[DRIVER_VERSION_BUF_LEN];
	int ret = -1;

	switch(mode)
	{
	case SETTING_USB_DEFAULT_MODE:
	case SETTING_USB_NONE_MODE:
		USB_LOG("Mode : SETTING_USB_DEFAULT_MODE mode_set_kernel\n");
		snprintf(buf, KERNEL_SET_BUF_SIZE, "%d", KERNEL_DEFAULT_MODE);
		break;
	case SETTING_USB_DEBUG_MODE:
		USB_LOG("Mode : USB_DEBUG_MODE mode_set_kernel\n");
		snprintf(buf, KERNEL_SET_BUF_SIZE, "%d", KERNEL_DEBUG_MODE);
		break;
	case SETTING_USB_ACCESSORY_MODE:
		USB_LOG("Mode : USB accessory is not supported in USB driver 0.0(C210)\n");
		return 0;
	default:
		USB_LOG("ERROR : parameter is not available(mode : %d)\n", mode);
		__USB_FUNC_EXIT__ ;
		return -1;
	}

	ret = write_file(KERNEL_SET_PATH, buf);
	um_retvm_if (EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", KERNEL_SET_PATH);

	__USB_FUNC_EXIT__ ;
	return 0;
}
static int driver_1_0_kernel_node_set(char *vendor_id, char *product_id, char *functions,
								char *device_class, char *device_subclass, char* device_protocol)
{
	__USB_FUNC_ENTER__ ;
	int ret = -1;

	if (vendor_id == NULL || product_id == NULL || functions == NULL
			|| device_class == NULL || device_subclass == NULL || device_protocol == NULL) {
		USB_LOG("ERROR: There are parameters whose value is NULL\n");
		return -1;
	}

	ret = write_file(USB_MODE_ENABLE, "0");
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_MODE_ENABLE);

	ret = write_file(USB_VENDOR_ID, vendor_id);
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_VENDOR_ID); 

	ret = write_file(USB_PRODUCT_ID, product_id);
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_PRODUCT_ID);

	ret = write_file(USB_FUNCTIONS, functions);
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_FUNCTIONS);

	ret = write_file(USB_DEVICE_CLASS, device_class);
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_DEVICE_CLASS);

	ret = write_file(USB_DEVICE_SUBCLASS, device_subclass);
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_DEVICE_SUBCLASS);

	ret = write_file(USB_DEVICE_PROTOCOL, device_protocol);
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_DEVICE_PROTOCOL);

	ret = write_file(USB_MODE_ENABLE, "1");
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_MODE_ENABLE);

	__USB_FUNC_EXIT__ ;
	return 0;
}

static int mode_set_driver_1_0(int mode)
{
	__USB_FUNC_ENTER__ ;

	int ret = -1;

	switch(mode)
	{
	case SETTING_USB_DEFAULT_MODE:
		USB_LOG("Mode : SETTING_USB_DEFAULT mode_set_kernel\n");
		ret = driver_1_0_kernel_node_set("04e8", "6860", "mtp,acm,sdb", "239", "2", "1");
		__USB_FUNC_EXIT__ ;
		return ret;

	case SETTING_USB_DEBUG_MODE:
		USB_LOG("Mode : USB_DEBUG_MODE mode_set_kernel\n");
		ret = driver_1_0_kernel_node_set("04e8", "6863", "rndis", "239", "2", "1");
		__USB_FUNC_EXIT__ ;
		return ret;

	case SETTING_USB_ACCESSORY_MODE:
		USB_LOG("Mode : USB_ACCESSORY_MODE mode_set_kernel\n");
		ret = driver_1_0_kernel_node_set("18d1", "2d00", "accessory", "0", "0", "0");
		__USB_FUNC_EXIT__ ;
		return ret;

	case SETTING_USB_NONE_MODE:
		USB_LOG("Mode : USB_NONE_MODE mode_set_kernel\n");
		ret = write_file(USB_MODE_ENABLE, "0");
		um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_MODE_ENABLE);
		__USB_FUNC_EXIT__ ;
		return ret;

	default:
		USB_LOG("ERROR : parameter is not available(mode : %d)\n", mode);
		__USB_FUNC_EXIT__ ;
		return ret;
	}
}

void start_dr(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	if(!ad) return ;
	call_cmd(CMD_DR_START);
	__USB_FUNC_EXIT__ ;
}

static Eina_Bool write_file(const char *filepath, char *content)
{
	__USB_FUNC_ENTER__ ;

	if(!filepath || !content) return EINA_FALSE;
	FILE *fp;
	int ret = -1;

	fp = fopen(filepath, "w");
	um_retvm_if (fp == NULL, EINA_FALSE, "FAIL: fopen(%s)\n", filepath);

	ret = fwrite(content, sizeof(char), strlen(content), fp);
	if ( ret < strlen(content)) {
		USB_LOG("FAIL: fwrite()\n");
		ret = fclose(fp);
		if(ret != 0) USB_LOG("FAIL : fclose()\n");
		return EINA_FALSE;
	}

	ret = fclose(fp);
	um_retvm_if (ret != 0, EINA_FALSE, "FAIL: result of fclose() is %d\n", ret);

	__USB_FUNC_EXIT__ ;
	return EINA_TRUE;
}

void load_connection_popup(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;

	if(!ad) return ;
	bundle *b = NULL;
	int ret = -1;
	int usbCurMode = -1;

	b = bundle_create();
	um_retm_if (!b, "FAIL: bundle_create()\n");

	ret = bundle_add(b, "0", "info");	/* "0" means tickernoti style */
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add()\n");
		if ( 0 != bundle_free(b) )
			USB_LOG("FAIL: bundle_free()\n");
		return;
	}

	ret = vconf_get_int(VCONFKEY_SETAPPL_USB_MODE_INT, &usbCurMode);
	USB_LOG("usbCurMode: %d\n", usbCurMode);
	switch(usbCurMode) {
	case SETTING_USB_DEFAULT_MODE:
		ret = bundle_add(b, "1", dgettext(USB_SERVER_MESSAGE_DOMAIN, USB_DEFAULT_MODE_CONNECTED));
		break;
	case SETTING_USB_DEBUG_MODE:
		ret = bundle_add(b, "1", dgettext(USB_SERVER_MESSAGE_DOMAIN,
								"IDS_HS_HEADER_USB_DEBUGGING_CONNECTED"));
		break;
	case SETTING_USB_MOBILE_HOTSPOT:
		ret = bundle_add(b, "1", dgettext(USB_SERVER_MESSAGE_DOMAIN,
								"IDS_ST_HEADER_USB_TETHERING_ENABLED"));
		break;
	case SETTING_USB_ACCESSORY_MODE:
		ret = bundle_add(b, "1", dgettext(USB_SERVER_MESSAGE_DOMAIN, USB_ACCESSORY_MODE_ENABLED));
		break;

	default:
		ret = bundle_add(b, "1", dgettext(USB_SERVER_MESSAGE_DOMAIN, USB_NOTICE_SYSPOPUP_FAIL));
		break;
	}
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add()\n");
		if ( 0 != bundle_free(b) )
			USB_LOG("FAIL: bundle_free()\n");
		return;
	}

	ret = bundle_add(b, "2", "1");	/* "2" means orientation of tickernoti */
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add()\n");
		if ( 0 != bundle_free(b) )
			USB_LOG("FAIL: bundle_free()\n");
		return;
	}

	ret = bundle_add(b, "3", "3");	/* "3" means timeout(second) of tickernoti */
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add()\n");
		if ( 0 != bundle_free(b) )
			USB_LOG("FAIL: bundle_free()\n");
		return;
	}

	ret = syspopup_launch(TICKERNOTI_SYSPOPUP, b);
	if (0 != ret) {
		USB_LOG("FAIL: syspopup_launch()\n");
	}

	ret = bundle_free(b);
	um_retm_if(0 != ret, "FAIL: bundle_free()\n");

	__USB_FUNC_EXIT__ ;
}
