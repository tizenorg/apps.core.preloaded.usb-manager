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

#include "um_customize.h"

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
		} else if (strncmp(buffer, DRIVER_VERSION_1_1, strlen(DRIVER_VERSION_1_1)) == 0) {
			USB_LOG("The driver version is 1.1 \n");
			ad->driverVersion = USB_DRIVER_1_1;
		} else {
			USB_LOG("This kernel version is unknown\n");
			return -1;
		}
	}
	__USB_FUNC_EXIT__ ;
	return 0;
}

int mode_set_kernel(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__ ;
	if (!ad) return -1;
	int ret = -1;
	switch(ad->driverVersion) {
	case USB_DRIVER_0_0:
		USB_LOG("USB driver version is 0.0");
		ret = mode_set_driver_0_0(mode);
		um_retvm_if (0 != ret, -1, "FAIL: mode_set_driver_0_0(mode)\n");
		break;
	case USB_DRIVER_1_0:
		USB_LOG("USB driver version is 1.0");
		ret = mode_set_driver_1_0(ad, mode);
		um_retvm_if (0 != ret, -1, "FAIL: mode_set_driver_1_0(mode)\n");
		break;
	case USB_DRIVER_1_1:
		USB_LOG("USB driver version is 1.1");
		ret = mode_set_driver_1_1(ad, mode);
		um_retvm_if (0 != ret, -1, "FAIL: mode_set_driver_1_1(mode)\n")
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
	case SETTING_USB_ETHERNET_MODE:
		USB_LOG("Mode : USB_ETHERNET_MODE mode_set_kernel\n");
		snprintf(buf, KERNEL_SET_BUF_SIZE, "%d", KERNEL_ETHERNET_MODE);
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

static int driver_1_0_kernel_node_set(char *vendor_id,
				      char *product_id,
				      char *functions,
				      char *device_class,
				      char *device_subclass,
				      char *device_protocol)
{
	__USB_FUNC_ENTER__ ;
	if (vendor_id == NULL || product_id == NULL || device_class == NULL
		|| functions == NULL || device_subclass == NULL || device_protocol == NULL) {
		USB_LOG("ERROR: There are parameters whose value is NULL\n");
		return -1;
	}
	int ret = -1;

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

static int mode_set_driver_1_0(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__ ;
	if (!ad) return -1;
	int ret = -1;

	switch(mode)
	{
	case SETTING_USB_DEFAULT_MODE:
		USB_LOG("Mode : SETTING_USB_DEFAULT_MODE mode_set_kernel\n");
#ifndef SIMULATOR
		if (ad->curDebugMode) {
			ret = driver_1_0_kernel_node_set("04e8", "6860", "mtp,acm,sdb",
							"239", "2", "1");
		} else {
			ret = driver_1_0_kernel_node_set("04e8", "6860", "mtp,acm",
							"239", "2", "1");
		}
#else
		ret = driver_1_0_kernel_node_set("04e8", "6860", "mtp,acm,sdb",
							"239", "2", "1");
#endif
		__USB_FUNC_EXIT__ ;
		return ret;

	case SETTING_USB_ETHERNET_MODE:
	case SETTING_USB_TETHERING_MODE:
		USB_LOG("Mode : USB_ETHERNET or TETHERING_MODE mode_set_kernel\n");
		ret = driver_1_0_kernel_node_set("04e8", "6864", "rndis", "239", "2", "1");
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
		return 0;

	default:
		USB_LOG("ERROR : parameter is not available(mode : %d)\n", mode);
		__USB_FUNC_EXIT__ ;
		return ret;
	}
}

static int driver_1_1_kernel_node_set(char *vendor_id,
				      char *product_id,
				      char *funcs_fconf,
				      char *funcs_sconf,
				      char *device_class,
				      char *device_subclass,
				      char *device_protocol)
{
	__USB_FUNC_ENTER__ ;
	if (vendor_id == NULL || product_id == NULL || funcs_fconf == NULL
		|| funcs_sconf == NULL || device_class == NULL
		|| device_subclass == NULL || device_protocol == NULL) {
		USB_LOG("ERROR: There are parameters whose value is NULL\n");
		return -1;
	}
	int ret = -1;

	ret = write_file(USB_MODE_ENABLE, "0");
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_MODE_ENABLE);

	ret = write_file(USB_VENDOR_ID, vendor_id);
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_VENDOR_ID);

	ret = write_file(USB_PRODUCT_ID, product_id);
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_PRODUCT_ID);

	ret = write_file(USB_FUNCS_FCONF, funcs_fconf);
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_FUNCS_FCONF);

	ret = write_file(USB_FUNCS_SCONF, funcs_sconf);
	um_retvm_if(EINA_FALSE == ret, -1, "FAIL: write_file(%s)\n", USB_FUNCS_SCONF);

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

static int mode_set_driver_1_1(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__ ;
	if (!ad) return -1;
	int ret = -1;

	switch(mode)
	{
	case SETTING_USB_DEFAULT_MODE:
		USB_LOG("Mode : SETTING_USB_DEFAULT mode_set_kernel\n");
#ifndef SIMULATOR
		if (ad->curDebugMode) {
			ret = driver_1_1_kernel_node_set("04e8", "6860", "mtp", "mtp,acm,sdb",
								"239", "2", "1");
		} else {
			ret = driver_1_1_kernel_node_set("04e8", "6860", "mtp", "mtp,acm",
								"239", "2", "1");
		}
#else
		ret = driver_1_1_kernel_node_set("04e8", "6860", "mtp", "mtp,acm,sdb",
								"239", "2", "1");
#endif
		__USB_FUNC_EXIT__ ;
		return ret;

	case SETTING_USB_ETHERNET_MODE:
	case SETTING_USB_TETHERING_MODE:
		USB_LOG("Mode : USB_ETHERNET or TETHERING_MODE mode_set_kernel\n");
		ret = driver_1_1_kernel_node_set("04e8", "6864", "rndis", " ",
						"239", "2", "1");
		__USB_FUNC_EXIT__ ;
		return ret;

	case SETTING_USB_ACCESSORY_MODE:
		USB_LOG("Mode : USB_ACCESSORY_MODE mode_set_kernel\n");
		ret = driver_1_1_kernel_node_set("18d1", "2d00", "accessory", " ",
						"0", "0", "0");
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

static int get_device_desc_info(char *buf, char *descType, int *descValue)
{
	__USB_FUNC_ENTER__ ;
	if (!buf || !descType || !descValue) return -1;
	char *found = NULL;
	char *tmp = NULL;

	found = strstr(buf, descType);
	if (found == NULL) {
		USB_LOG("FAIL: strnstr(%s)", descType);
		return -1;
	}
	found = found + strlen(descType);
	while(*found == ' ') {
		found++;
	}
	tmp = strstr(found, " ");
	if (tmp != NULL) *tmp = '\0';

	*descValue = strtoul(found, &tmp, 16);
	__USB_FUNC_EXIT__ ;
	return 0;
}

int store_default_device_info(	UmMainData *ad,
										int class,
										int subClass,
										int protocol,
										int vendor,
										int product,
										int bus,
										int devAddress)
{
	__USB_FUNC_ENTER__ ;
	UsbHost *device = (UsbHost *)malloc(sizeof(UsbHost));
	device->permittedAppId = NULL;
	device->deviceClass = class;
	device->deviceSubClass = subClass;
	device->deviceProtocol = protocol;
	device->idVendor = vendor;
	device->idProduct = product;
	device->bus = bus;
	device->deviceAddress = devAddress;
	ad->defaultDevList = g_list_append(ad->defaultDevList, device);
	__USB_FUNC_EXIT__ ;
	return 0;
}

int get_default_usb_device(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	if (!ad) return -1;
	FILE *fp;
	int ret = -1;
	char buf[BUF_MAX];
	bool checkIF = false;
	char *found = NULL;
	int defClass = -1;
	int defSubClass = -1;
	int defProtocol = -1;
	int defVendor = -1;
	int defProduct = -1;
	int defBus = -1;
	int defDevAddress = -1;

	ret = access("/tmp/usb_default", F_OK);
	/*  The file does not exist if default usb device does not exist */
	um_retvm_if(ret < 0, 0, "usb_default file does not exist.");

	fp = fopen("/tmp/usb_default", "r");
	um_retvm_if(fp == NULL, -1, "FAIL: fopen()");

	FREE(ad->defaultDevList);

	while(fgets(buf, BUF_MAX, fp)) {
		USB_LOG("BUF: %s", buf);

		/* 'D' means device descriptor which has information of class, subclass, and protocol */
		/* 'P' also means device descriptor which has information of vendor and product */
		/* 'I' means interface descriptor which has information of class, subclass, and protocol */
		/* 'T' means Topology of devices which has information of bus number and device address */
		if (*buf == 'D' || *buf == 'I') {
			if (*buf == 'I' && checkIF == false) {
				USB_LOG("Device class is located in the device descriptor");
				continue;
			}

			USB_LOG("Found the information of class, subclass, and protocol");
			/* Getting Protocol */
			ret = get_device_desc_info(buf, "Prot=", &defProtocol);
			if (ret < 0) USB_LOG("FAIL: get_device_desc_info(Protocol)");

			/* Getting SubClass */
			ret = get_device_desc_info(buf, "Sub=", &defSubClass);
			if (ret < 0) USB_LOG("FAIL: get_device_desc_info(SubClass)");

			/* Getting Class */
			ret = get_device_desc_info(buf, "Cls=", &defClass);
			if (ret < 0) USB_LOG("FAIL: get_device_desc_info(Class)");

			/* Deciding whether or not checking interface */
			if (*buf == 'D' && defClass <= 0) {
				USB_LOG("Device class is located in the interface decsriptor");
				checkIF = true;
			} else if (*buf == 'I' && checkIF == true) {
				USB_LOG("Device class is stored from interface descriptor");
				checkIF = false;
			}
		} else if (*buf == 'T') {
			USB_LOG("Found the information of bus number and device address");

			/* Getting Bus number */
			ret = get_device_desc_info(buf, "Dev#=", &defDevAddress);
			if (ret < 0) USB_LOG("FAIL: get_device_desc_info(devAddress)");

			/* Getting device address */
			ret = get_device_desc_info(buf, "Bus=", &defBus);
			if (ret < 0) USB_LOG("FAIL: get_device_desc_info(bus)");

		} else if (*buf == 'P') {
			USB_LOG("Found the information of vendor and product");
			/* Getting Product ID */
			ret = get_device_desc_info(buf, "ProdID=", &defProduct);
			if (ret < 0) USB_LOG("FAIL: get_device_desc_info(ProductID)");

			/* Getting Vendor ID */
			ret = get_device_desc_info(buf, "Vendor=", &defVendor);
			if (ret < 0) USB_LOG("FAIL: get_device_desc_info(Vendor)");

		} else {
			USB_LOG("Cannot find device information");
		}

		if (defClass > 0 && defVendor >= 0) {
			ret = store_default_device_info(ad, defClass, defSubClass, defProtocol,
										defVendor, defProduct, defBus, defDevAddress);
			if (ret < 0) {
				USB_LOG("FAIL: store_default_device_info()");
			}
			defClass = -1;
			defSubClass = -1;
			defProtocol = -1;
			defVendor = -1;
			defProduct = -1;
			defBus = -1;
			defDevAddress = -1;
		}
	}

	fclose(fp);
	USB_LOG("Number of default devices: %d", g_list_length(ad->defaultDevList));

	__USB_FUNC_EXIT__ ;
	return 0;
}

bool is_device_supported(int class)
{
	__USB_FUNC_ENTER__ ;
	switch (class) {
	case USB_HOST_CDC:
	case USB_HOST_HID:
	case USB_HOST_CAMERA:
	case USB_HOST_PRINTER:
	case USB_HOST_MASS_STORAGE:
	case USB_HOST_HUB:
		__USB_FUNC_EXIT__ ;
		return true;
	default:
		 __USB_FUNC_EXIT__ ;
		return false;
	}
}

void load_connection_popup(UmMainData *ad, char *msg, int orientation)
{
	__USB_FUNC_ENTER__ ;

	if(!ad) return ;
	if(!msg) return ;
	bundle *b = NULL;
	int ret = -1;
	int usbCurMode = -1;
	const int arrSize = 2;
	char str_orientation[arrSize];

	USB_LOG("msg: %s", msg);

	b = bundle_create();
	um_retm_if (!b, "FAIL: bundle_create()\n");

	/* Set tickernoti style */
	ret = bundle_add(b, "0", "info");	/* "0" means tickernoti style */
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add()\n");
		if ( 0 != bundle_free(b) )
			USB_LOG("FAIL: bundle_free()\n");
		return;
	}

	/* Set tickernoti text */
	ret = bundle_add(b, "1", dgettext(USB_SERVER_MESSAGE_DOMAIN, msg)); /* "1" means popup text */
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add()\n");
		if ( 0 != bundle_free(b) )
			USB_LOG("FAIL: bundle_free()\n");
		return;
	}

	/* Set tickernoti orientation */
	snprintf(str_orientation, arrSize, "%d", orientation);
	ret = bundle_add(b, "2", str_orientation);  /* "2" means orientation of tickernoti(1: bottom) */
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add()\n");
		if ( 0 != bundle_free(b) )
			USB_LOG("FAIL: bundle_free()\n");
		return;
	}

	/* Set tickernoti timeout */
	ret = bundle_add(b, "3", "3");	/* "3" means timeout(second) of tickernoti */
	if (0 != ret) {
		USB_LOG("FAIL: bundle_add()\n");
		if ( 0 != bundle_free(b) )
			USB_LOG("FAIL: bundle_free()\n");
		return;
	}

	ret = syspopup_launch(TICKERNOTI_SYSPOPUP, b);
	USB_LOG("ret: %d", ret);
	if (0 > ret) {
		USB_LOG("FAIL: syspopup_launch()\n");
	}

	ret = bundle_free(b);
	um_retm_if(0 != ret, "FAIL: bundle_free()\n");

	usleep(1000000);

	__USB_FUNC_EXIT__ ;
}
