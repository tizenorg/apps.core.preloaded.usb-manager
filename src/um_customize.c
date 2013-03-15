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

#define UNLOAD_WAIT 1000000 /* Wait for 1 sec */
#define LAUNCH_WAIT 100000 /* Wait for 0.1 sec */
#define LAUNCH_RETRY 10

int call_cmd(char* cmd)
{
	__USB_FUNC_ENTER__ ;
	assert(cmd);
	int ret = system(cmd);
	USB_LOG("The result of %s is %d\n",cmd, ret);
	__USB_FUNC_EXIT__ ;
	return ret;
}

/* If other kernel versions are added, we should modify this function */
int check_driver_version(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);

	char buffer[DRIVER_VERSION_BUF_LEN];
	FILE *fp;

	if (access(DRIVER_VERSION_PATH, F_OK) != 0) {
		USB_LOG("This kernel is for version 2.6x");
		ad->driverVersion = USB_DRIVER_0_0;
		__USB_FUNC_EXIT__ ;
		return 0;
	}

	fp = fopen(DRIVER_VERSION_PATH, "r");
	um_retvm_if(fp == NULL, -1, "FAIL: fopen(%s)", DRIVER_VERSION_PATH);

	if (fgets(buffer, sizeof(buffer), fp) == NULL) {
		USB_LOG("FAIL: fgets(%s)", DRIVER_VERSION_PATH);
		if (fclose(fp) != 0) {
			USB_LOG("FAIL: fclose(fp)\n");
		}
		return -1;
	}

	if (fclose(fp) != 0) {
		USB_LOG("FAIL: fclose(fp)");
		return -1;
	}

	if (!strncmp(buffer, DRIVER_VERSION_1_0, strlen(DRIVER_VERSION_1_0))) {
		USB_LOG("The driver version is 1.0");
		ad->driverVersion = USB_DRIVER_1_0;
		__USB_FUNC_EXIT__ ;
		return 0;
	}

	if (!strncmp(buffer, DRIVER_VERSION_1_1, strlen(DRIVER_VERSION_1_1))) {
		USB_LOG("The driver version is 1.1");
		ad->driverVersion = USB_DRIVER_1_1;
		__USB_FUNC_EXIT__ ;
		return 0;
	}

	USB_LOG("This kernel version is unknown");
	__USB_FUNC_EXIT__ ;
	return -1;
}

static int write_file(const char *filepath, char *content)
{
	__USB_FUNC_ENTER__ ;
	assert(filepath);
	assert(content);

	FILE *fp;
	int ret;

	fp = fopen(filepath, "w");
	um_retvm_if(!fp, -1, "FAIL: fopen(%s)", filepath);

	ret = fwrite(content, sizeof(char), strlen(content), fp);
	if (ret < strlen(content)) {
		USB_LOG("FAIL: fwrite(): %d", ret);
		ret = fclose(fp);
		if(0 != ret)
			USB_LOG("FAIL : fclose()");
		return -1;
	}

	ret = fclose(fp);
	um_retvm_if(0 != ret, -1, "FAIL: result of fclose() is %d", ret);

	__USB_FUNC_EXIT__ ;
	return 0;
}

static int mode_set_driver_0_0(int mode)
{
	__USB_FUNC_ENTER__ ;

	char buf[DRIVER_VERSION_BUF_LEN];
	int ret;

	switch(mode) {
	case SET_USB_DEFAULT:
	case SET_USB_SDB:
	case SET_USB_SDB_DIAG:
	case SET_USB_NONE:
		USB_LOG("Mode : SET_USB_DEFAULT mode_set_kernel");
		snprintf(buf, KERNEL_SET_BUF_SIZE, "%d", KERNEL_DEFAULT_MODE);
		break;
	case SET_USB_RNDIS:
	case SET_USB_RNDIS_TETHERING:
	case SET_USB_RNDIS_SDB:
		USB_LOG("Mode : USB_ETHERNET_MODE mode_set_kernel");
		snprintf(buf, KERNEL_SET_BUF_SIZE, "%d", KERNEL_ETHERNET_MODE);
		break;
	case SET_USB_ACCESSORY:
		USB_LOG("Mode : USB accessory is not supported in USB driver 0.0");
		return 0;
	default:
		USB_LOG("ERROR : parameter is not available(mode : %d)", mode);
		__USB_FUNC_EXIT__ ;
		return -1;
	}

	ret = write_file(KERNEL_SET_PATH, buf);
	um_retvm_if (0 > ret, -1, "FAIL: write_file(%s)", KERNEL_SET_PATH);

	__USB_FUNC_EXIT__ ;
	return 0;
}

static int driver_1_0_kernel_node_set(char *vendor_id,
				      char *product_id,
				      char *functions,
				      char *device_class,
				      char *device_subclass,
				      char *device_protocol,
				      char *opt_diag)
{
	__USB_FUNC_ENTER__ ;
	assert(vendor_id);
	assert(product_id);
	assert(functions);
	assert(device_class);
	assert(device_subclass);
	assert(device_protocol);

	int ret = -1;

	ret = write_file(USB_MODE_ENABLE, "0");
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_MODE_ENABLE);

	ret = write_file(USB_VENDOR_ID, vendor_id);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_VENDOR_ID);

	ret = write_file(USB_PRODUCT_ID, product_id);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_PRODUCT_ID);

	ret = write_file(USB_FUNCTIONS, functions);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_FUNCTIONS);

	ret = write_file(USB_DEVICE_CLASS, device_class);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_DEVICE_CLASS);

	ret = write_file(USB_DEVICE_SUBCLASS, device_subclass);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_DEVICE_SUBCLASS);

	ret = write_file(USB_DEVICE_PROTOCOL, device_protocol);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_DEVICE_PROTOCOL);

	if (opt_diag) {
		ret = write_file(USB_DIAG_CLIENT, opt_diag);
		um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_DIAG_CLIENT);
	}

	ret = write_file(USB_MODE_ENABLE, "1");
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_MODE_ENABLE);

	__USB_FUNC_EXIT__ ;
	return 0;
}

static int mode_set_driver_1_0(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	int ret;

	switch(mode) {
	case SET_USB_DEFAULT:
#ifndef SIMULATOR
		ret = driver_1_0_kernel_node_set("04e8", "6860",
						"mtp,acm", "239", "2", "1", NULL);
#else
		ret = driver_1_0_kernel_node_set("04e8", "6860",
						"mtp,acm,sdb", "239", "2", "1", NULL);
#endif
		__USB_FUNC_EXIT__ ;
		return ret;

	case SET_USB_SDB:
		ret = driver_1_0_kernel_node_set("04e8", "6860",
						"mtp,acm,sdb", "239", "2", "1", NULL);
		__USB_FUNC_EXIT__ ;
		return ret;

	case SET_USB_SDB_DIAG:
		ret = driver_1_0_kernel_node_set("04e8", "6860", "mtp,acm,sdb,diag",
						"239", "2", "1", "diag_mdm");
		__USB_FUNC_EXIT__ ;
		return ret;

	case SET_USB_RNDIS:
	case SET_USB_RNDIS_TETHERING:
		ret = driver_1_0_kernel_node_set("04e8", "6864",
						"rndis", "239", "2", "1", NULL);
		__USB_FUNC_EXIT__ ;
		return ret;

	case SET_USB_RNDIS_SDB:
		ret = driver_1_0_kernel_node_set("04e8", "6864",
						"rndis,sdb", "239", "2", "1", NULL);
		__USB_FUNC_EXIT__ ;
		return ret;

	case SET_USB_ACCESSORY:
		ret = driver_1_0_kernel_node_set("18d1", "2d00",
						"accessory", "0", "0", "0", NULL);
		__USB_FUNC_EXIT__ ;
		return ret;

	case SET_USB_NONE:
		ret = write_file(USB_MODE_ENABLE, "0");
		um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_MODE_ENABLE);
		__USB_FUNC_EXIT__ ;
		return 0;

	default:
		USB_LOG("ERROR : parameter is not available(mode : %d)", mode);
		__USB_FUNC_EXIT__ ;
		return -1;
	}
}

static int driver_1_1_kernel_node_set(char *vendor_id,
				      char *product_id,
				      char *funcs_fconf,
				      char *funcs_sconf,
				      char *device_class,
				      char *device_subclass,
				      char *device_protocol,
				      char *opt_diag)
{
	__USB_FUNC_ENTER__ ;
	assert(vendor_id);
	assert(product_id);
	assert(funcs_fconf);
	assert(funcs_sconf);
	assert(device_class);
	assert(device_subclass);
	assert(device_protocol);

	int ret;

	ret = write_file(USB_MODE_ENABLE, "0");
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_MODE_ENABLE);

	ret = write_file(USB_VENDOR_ID, vendor_id);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_VENDOR_ID);

	ret = write_file(USB_PRODUCT_ID, product_id);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_PRODUCT_ID);

	ret = write_file(USB_FUNCS_FCONF, funcs_fconf);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_FUNCS_FCONF);

	ret = write_file(USB_FUNCS_SCONF, funcs_sconf);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_FUNCS_SCONF);

	ret = write_file(USB_DEVICE_CLASS, device_class);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_DEVICE_CLASS);

	ret = write_file(USB_DEVICE_SUBCLASS, device_subclass);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_DEVICE_SUBCLASS);

	ret = write_file(USB_DEVICE_PROTOCOL, device_protocol);
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_DEVICE_PROTOCOL);

	if (opt_diag) {
		ret = write_file(USB_DIAG_CLIENT, opt_diag);
		um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_DIAG_CLIENT);
	}

	ret = write_file(USB_MODE_ENABLE, "1");
	um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_MODE_ENABLE);

	__USB_FUNC_EXIT__ ;
	return 0;
}

static int mode_set_driver_1_1(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	int ret;

	switch(mode)
	{
	case SET_USB_DEFAULT:
#ifndef SIMULATOR
		ret = driver_1_1_kernel_node_set("04e8", "6860",
						"mtp", "mtp,acm", "239", "2", "1", NULL);
#else
		ret = driver_1_1_kernel_node_set("04e8", "6860",
						"mtp", "mtp,acm,sdb", "239", "2", "1", NULL);
#endif
		__USB_FUNC_EXIT__ ;
		return ret;

	case SET_USB_SDB:
		ret = driver_1_1_kernel_node_set("04e8", "6860",
						"mtp", "mtp,acm,sdb", "239", "2", "1", NULL);
		__USB_FUNC_EXIT__ ;
		return ret;

	case SET_USB_SDB_DIAG:
		ret = driver_1_1_kernel_node_set("04e8", "6860", "mtp", "mtp,acm,sdb,diag",
						"239", "2", "1", "diag_mdm");
		__USB_FUNC_EXIT__ ;
		return ret;

	case SET_USB_RNDIS:
	case SET_USB_RNDIS_TETHERING:
		ret = driver_1_1_kernel_node_set("04e8", "6864",
						"rndis", " ", "239", "2", "1", NULL);
		__USB_FUNC_EXIT__ ;
		return ret;

	case SET_USB_RNDIS_SDB:
		ret = driver_1_1_kernel_node_set("04e8", "6864",
						"rndis,sdb", " ", "239", "2", "1", NULL);
		__USB_FUNC_EXIT__ ;
		return ret;

	case SET_USB_ACCESSORY:
		ret = driver_1_1_kernel_node_set("18d1", "2d00",
						"accessory", " ", "0", "0", "0", NULL);
		__USB_FUNC_EXIT__ ;
		return ret;

	case SET_USB_NONE:
		ret = write_file(USB_MODE_ENABLE, "0");
		um_retvm_if(0 > ret, -1, "FAIL: write_file(%s)", USB_MODE_ENABLE);
		__USB_FUNC_EXIT__ ;
		return 0;

	default:
		USB_LOG("ERROR : parameter is not available(mode : %d)", mode);
		__USB_FUNC_EXIT__ ;
		return -1;
	}
}

int mode_set_kernel(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);

	int ret;

	switch(ad->driverVersion) {
	case USB_DRIVER_0_0:
		USB_LOG("USB driver version is 0.0");
		ret = mode_set_driver_0_0(mode);
		um_retvm_if (0 != ret, -1, "FAIL: mode_set_driver_0_0(mode)");
		break;
	case USB_DRIVER_1_0:
		USB_LOG("USB driver version is 1.0");
		ret = mode_set_driver_1_0(ad, mode);
		um_retvm_if (0 != ret, -1, "FAIL: mode_set_driver_1_0(mode)");
		break;
	case USB_DRIVER_1_1:
		USB_LOG("USB driver version is 1.1");
		ret = mode_set_driver_1_1(ad, mode);
		um_retvm_if (0 != ret, -1, "FAIL: mode_set_driver_1_1(mode)")
		break;

		/* If other driver versions are added,
		 * add functions here that notice USB mode to the kernel */

	default:
		USB_LOG("This version of driver is not registered to usb-server");
		__USB_FUNC_EXIT__ ;
		return -1;
	}
	__USB_FUNC_EXIT__ ;
	return 0;
}

static int get_device_desc_info(char *buf, char *descType)
{
	__USB_FUNC_ENTER__ ;
	assert(buf);
	assert(descType);

	char *found;
	char *tmp;
	int descValue;

	found = strstr(buf, descType);
	if (!found) {
		USB_LOG("FAIL: strnstr(%s)", descType);
		return -1;
	}

	found = found + strlen(descType);
	while(*found == ' ') {
		found++;
	}

	tmp = strstr(found, " ");
	if (tmp) *tmp = '\0';

	descValue = strtoul(found, &tmp, 16);
	USB_LOG("%s: %d", descType, descValue);

	__USB_FUNC_EXIT__ ;
	return descValue;
}

static int store_default_device_info(UmMainData *ad, UsbInterface *devIf)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(devIf);

	UsbInterface *device = (UsbInterface *)malloc(sizeof(UsbInterface));
	if (!device) {
		USB_LOG("FAIL: malloc()");
		return -1;
	}

	device->permAppId = NULL;
	device->ifClass = devIf->ifClass;
	device->ifSubClass = devIf->ifSubClass;
	device->ifProtocol = devIf->ifProtocol;
	device->ifIdVendor = devIf->ifIdVendor;
	device->ifIdProduct = devIf->ifIdProduct;
	device->ifBus = devIf->ifBus;
	device->ifAddress = devIf->ifAddress;
	device->ifNumber = devIf->ifNumber;

	ad->defaultDevList = g_list_append(ad->defaultDevList, device);

	__USB_FUNC_EXIT__ ;
	return 0;
}

static void get_device_info_from_device_desc(char *buf, UsbInterface *devIf)
{
	__USB_FUNC_ENTER__ ;
	assert(buf);
	assert(devIf);

	/* Getting Protocol */
	devIf->ifProtocol = get_device_desc_info(buf, "Prot=");
	if (devIf->ifProtocol < 0) USB_LOG("FAIL: get_device_desc_info()");

	/* Getting SubClass */
	devIf->ifSubClass = get_device_desc_info(buf, "Sub=");
	if (devIf->ifSubClass < 0) USB_LOG("FAIL: get_device_desc_info()");

	/* Getting Class */
	devIf->ifClass = get_device_desc_info(buf, "Cls=");
	if (devIf->ifClass < 0) USB_LOG("FAIL: get_device_desc_info()");

	/* Getting Interface number */
	devIf->ifNumber = get_device_desc_info(buf, "If#=");
	if (devIf->ifNumber < 0) USB_LOG("FAIL: get_device_desc_info()");

	__USB_FUNC_EXIT__ ;
}

static void get_device_info_from_product_desc(char *buf, UsbInterface *devIf)
{
	__USB_FUNC_ENTER__ ;
	assert(buf);
	assert(devIf);

	/* Getting Product ID */
	devIf->ifIdProduct = get_device_desc_info(buf, "ProdID=");
	if (devIf->ifIdProduct < 0) USB_LOG("FAIL: get_device_desc_info()");

	/* Getting Vendor ID */
	devIf->ifIdVendor = get_device_desc_info(buf, "Vendor=");
	if (devIf->ifIdVendor < 0) USB_LOG("FAIL: get_device_desc_info(Vendor)");

	__USB_FUNC_EXIT__ ;
}

static void get_device_info_from_topology_desc(char *buf, UsbInterface *devIf)
{
	__USB_FUNC_ENTER__ ;
	assert(buf);
	assert(devIf);

	/* Getting device number */
	devIf->ifAddress = get_device_desc_info(buf, "Dev#=");
	if (devIf->ifAddress < 0) USB_LOG("FAIL: get_device_desc_info()");

	/* Getting bus address */
	devIf->ifBus = get_device_desc_info(buf, "Bus=");
	if (devIf->ifBus < 0) USB_LOG("FAIL: get_device_desc_info()");

	__USB_FUNC_EXIT__ ;
}

int get_default_usb_device(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	FILE *fp;
	int ret;
	char buf[BUF_MAX];
	UsbInterface devIf;

	ret = access("/tmp/usb_default", F_OK);
	/*  The file does not exist if default usb device does not exist */
	um_retvm_if(ret < 0, 0, "usb_default file does not exist.");

	fp = fopen("/tmp/usb_default", "r");
	um_retvm_if(!fp, -1, "FAIL: fopen()");

	while(fgets(buf, sizeof(buf), fp)) {
		USB_LOG("BUF: %s", buf);

	/* 'P' means device descriptor which has information of vendor and product */
	/* 'I' means interface descriptor which has information of class, subclass, and protocol */
	/* 'T' means Topology of devices which has information of bus number and device address */
		if (*buf == 'I') {
			get_device_info_from_device_desc(buf, &devIf);
			USB_LOG("interface number: %d, class: %d", devIf.ifNumber, devIf.ifClass);
			USB_LOG("subclass: %d, protocol: %d", devIf.ifSubClass, devIf.ifProtocol);

		} else if (*buf == 'T') {
			get_device_info_from_topology_desc(buf, &devIf);
			USB_LOG("bus: %d, devaddress: %d", devIf.ifBus, devIf.ifAddress);
			continue;

		} else if (*buf == 'P') {
			get_device_info_from_product_desc(buf, &devIf);
			USB_LOG("vendor: %d, product: %d", devIf.ifIdVendor, devIf.ifIdProduct);
			continue;

		} else {
			USB_LOG("Cannot find device information");
			continue;
		}

		ret = store_default_device_info(ad, &devIf);
		if (ret < 0) {
			USB_LOG("FAIL: store_default_device_info()");
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
	case USB_HOST_REF_INTERFACE:
	case USB_HOST_CDC:
	case USB_HOST_HID:
	case USB_HOST_CAMERA:
	case USB_HOST_PRINTER:
	case USB_HOST_MASS_STORAGE:
	case USB_HOST_HUB:
	case USB_HOST_MISCELLANEOUS:
	case USB_HOST_VENDOR_SPECIFIC:
		__USB_FUNC_EXIT__ ;
		return true;
	default:
		 __USB_FUNC_EXIT__ ;
		return false;
	}
}

void load_connection_popup(char *msg)
{
	__USB_FUNC_ENTER__ ;
	assert(msg);
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	noti_err = notification_status_message_post(gettext(msg));
	if (noti_err != NOTIFICATION_ERROR_NONE)
		USB_LOG("FAIL: notification_status_message_post(msg)");
	__USB_FUNC_EXIT__ ;
}
