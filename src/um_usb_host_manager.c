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

#define _GNU_SOURCE
#include <string.h>

#include <vconf.h>
#include <dirent.h>
#include <aul.h>
#include <ctype.h>
#include "um_usb_host_manager.h"

#define TIMER_WAIT 0.5 /* Wait for 0.5 sec */
#define FS_TMPFS "tmpfs"

#define USBOTG_SYSPOPUP "usbotg-syspopup"
#define STORAGE_ADD     "storage_add"
#define STORAGE_REMOVE  "storage_remove"
#define CAMERA_ADD      "camera_add"
#define CAMERA_REMOVE   "camera_remove"

#define DEV_STR_UNSUPPORTED "Unsupported Device"
#define DEV_STR_PRINTER     "Printer"
#define DEV_STR_KEYBOARD    "Keyboard"
#define DEV_STR_MOUSE       "Mouse"
#define DEV_STR_CAMERA      "Camera"

#define VENDOR_HP_1         "Hewlett-Packard"
#define VENDOR_HP_2         "Hewlett Packard"
#define VENDOR_HP_3         "HP"

static int um_usb_storage_added(UmMainData *ad, char *devpath, char *devname, char *fstype);
static int um_usb_storage_removed(UmMainData *ad, char *devname);
static int um_connect_known_device(UmMainData *ad, UsbInterface *device);
static void um_disconnect_usb_device_mass_storage(UmMainData *ad, char *devname);
static int um_disconnect_usb_device(UmMainData *ad, UsbInterface *device);
static int um_connect_unknown_device(UmMainData *ad, UsbInterface *device);

static void free_dev_list(gpointer data)
{
	__USB_FUNC_ENTER__;
	assert(data);
	UsbInterface *devIf = (UsbInterface *)data;
	FREE(devIf->permAppId);
	FREE(devIf->ifIProduct);
	FREE(devIf->ifIManufacturer);
	__USB_FUNC_EXIT__;
}

static void free_ms_list(gpointer data)
{
	__USB_FUNC_ENTER__;
	assert(data);
	MSDevName *msDevice = (MSDevName *)data;
	FREE(msDevice->devName);
	__USB_FUNC_EXIT__;
}

void show_all_usb_devices(GList *devList)
{
	__USB_FUNC_ENTER__;

	UsbInterface *devIf;
	GList *l;

	USB_LOG("***********************************");
	USB_LOG("** USB devices list ");
	USB_LOG("** ");

	for (l = devList ; l ; l = g_list_next(l)) {
		devIf = (UsbInterface *)(l->data);
		USB_LOG("** Permitted app : %s", devIf->permAppId);
		USB_LOG("** Device Bus: %u", devIf->ifBus);
		USB_LOG("** Device Address: %u", devIf->ifAddress);
		USB_LOG("** Interface number : %u", devIf->ifNumber);
		USB_LOG("** Interface Class : %u", devIf->ifClass);
		USB_LOG("** Interface Subclass : %u", devIf->ifSubClass);
		USB_LOG("** Interface Protocol : %u", devIf->ifProtocol);
		USB_LOG("** Interface Vendor ID : %u", devIf->ifIdVendor);
		USB_LOG("** Interface Product ID: %u", devIf->ifIdProduct);
		USB_LOG("*************************************** ");
	}

	__USB_FUNC_EXIT__;
}

static int get_ifs_from_device(libusb_device *device, GList **ifList)
{
	__USB_FUNC_ENTER__;
	assert(device);
	assert(ifList);

	int i;
	int ret;
	int numIf;
	struct libusb_config_descriptor *config = NULL;
	const struct libusb_interface_descriptor *descIf;
	UsbInterface *devIf;

	ret = libusb_get_active_config_descriptor(device, &config);
	if (ret != 0) {
		USB_LOG("FAIL: libusb_get_active_config_descriptor()");
		libusb_free_config_descriptor(config);
		return -1;
	}

	numIf = config->bNumInterfaces;
	USB_LOG("number of interfaces: %d", numIf);

	for (i = 0 ; i < numIf ; i++) {
		descIf = &(config->interface[i].altsetting[0]);
		devIf = (UsbInterface *)malloc(sizeof(UsbInterface));
		if (!devIf) {
			USB_LOG("FAIL: malloc()");
			libusb_free_config_descriptor(config);
			g_list_free(*ifList);
			return -1;
		}

		devIf->ifClass = descIf->bInterfaceClass;
		devIf->ifSubClass = descIf->bInterfaceSubClass;
		devIf->ifProtocol = descIf->bInterfaceProtocol;
		devIf->ifNumber = descIf->bInterfaceNumber;

		*ifList = g_list_append(*ifList, devIf);
	}

	libusb_free_config_descriptor(config);
	__USB_FUNC_EXIT__;
	return 0;
}

static int get_device_i_manufacturer_product(libusb_device *device,
					char **manufacturer,
					char **product)
{
	__USB_FUNC_ENTER__;
	assert(device);
	assert(manufacturer);
	assert(product);

	int ret = -1;
	libusb_device_handle *handle = NULL;
	struct libusb_device_descriptor desc;
	unsigned char devIProduct[BUF_MAX];
	unsigned char devIManufacturer[BUF_MAX];

	ret = libusb_get_device_descriptor(device, &desc);
	if (ret != 0) {
		USB_LOG("FAIL: libusb_get_device_descriptor()");
		return -1;
	}

	ret = libusb_open(device, &handle);
	if (ret != 0) {
		USB_LOG("FAIL: libusb_open()");
		return -1;
	}

	ret = libusb_get_string_descriptor_ascii(handle,
						desc.iProduct,
						devIProduct,
						sizeof(devIProduct));
	if (ret < 0) {
		USB_LOG("FAIL: libusb_get_string_descriptor_ascii(): %d", ret);
		libusb_close(handle);
		return -1;
	}

	ret = libusb_get_string_descriptor_ascii(handle,
						desc.iManufacturer,
						devIManufacturer,
						sizeof(devIManufacturer));
	if (ret < 0) {
		USB_LOG("FAIL: libusb_get_string_descriptor_ascii(): %d", ret);
		libusb_close(handle);
		return -1;
	}

	libusb_close(handle);
	USB_LOG("iManufacturer: %s, iProduct: %s", devIManufacturer, devIProduct);
	*manufacturer = strdup((const char *)devIManufacturer);
	*product = strdup((const char *)devIProduct);

	__USB_FUNC_EXIT__;
	return 0;
}

static int retrieve_interface_info(libusb_device *device, GList **ifList)
{
	__USB_FUNC_ENTER__;
	assert(device);
	assert(ifList);

	struct libusb_device_descriptor desc;
	GList *l;
	int ret;
	UsbInterface *devIf;

	ret = libusb_get_device_descriptor(device, &desc);
	if (ret != 0) {
		USB_LOG("FAIL: libusb_get_device_descriptor()");
		return -1;
	}

	ret = get_ifs_from_device(device, ifList);
	if (ret < 0) {
		USB_LOG("FAIL: get_ifs_from_device()");
		return -1;
	}

	for (l = *ifList ; l ; l = g_list_next(l)) {
		devIf = (UsbInterface *)(l->data);
		devIf->ifIdVendor = desc.idVendor;
		devIf->ifIdProduct = desc.idProduct;
		devIf->ifBus = libusb_get_bus_number(device);
		devIf->ifAddress = libusb_get_device_address(device);
		devIf->permAppId = NULL;

		ret = get_device_i_manufacturer_product(device,
							&(devIf->ifIManufacturer),
							&(devIf->ifIProduct));
		if (0 > ret) {
			USB_LOG("FAIL: get_device_i_manufacturer_product");
			devIf->ifIManufacturer = NULL;
			devIf->ifIProduct = NULL;
		}
	}

	__USB_FUNC_EXIT__;
	return 0;
}

static void um_release_all_device(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	int ret = -1;
	UsbInterface *dev = NULL;
	MSDevName *msDev = NULL;
	GList *l = NULL;

	if (ad->devList) {
		for (l = ad->devList ; l ; l = g_list_next(l)) {
			dev = (UsbInterface *)(l->data);
			ret = um_disconnect_usb_device(ad, dev);
			if (ret < 0)
				USB_LOG("FAIL: um_disconnect_usb_device()");
		}

		g_list_free_full(ad->devList, free_dev_list);
		ad->devList = NULL;
	}

	if (ad->devMSList) {
		for (l = ad->devMSList ; l ; l = g_list_next(l)) {
			msDev = (MSDevName *)(l->data);
			um_disconnect_usb_device_mass_storage(ad, msDev->devName);
		}

		g_list_free_full(ad->devMSList, free_ms_list);
		ad->devMSList = NULL;
	}

	ret = um_remove_all_host_notification(ad);
	if (ret < 0) USB_LOG("FAIL: um_remove_all_host_notification(ad)");

	if (ad->addHostTimer) {
		ecore_timer_del(ad->addHostTimer);
		ad->addHostTimer = NULL;
	}

	if (ad->rmHostTimer) {
		ecore_timer_del(ad->rmHostTimer);
		ad->rmHostTimer = NULL;
	}

	load_connection_popup("IDS_COM_POP_USB_CONNECTOR_DISCONNECTED");

	__USB_FUNC_EXIT__;
}

static bool find_same_device_interface(UsbInterface *dev1, UsbInterface *dev2)
{
	__USB_FUNC_ENTER__;
	assert(dev1);
	assert(dev2);

	if (dev1->ifBus != dev2->ifBus)
		return false;
	if (dev1->ifAddress != dev2->ifAddress)
		return false;
	if (dev1->ifClass != dev2->ifClass)
		return false;
	if (dev1->ifSubClass != dev2->ifSubClass)
		return false;
	if (dev1->ifProtocol != dev2->ifProtocol)
		return false;
	if (dev1->ifIdVendor != dev2->ifIdVendor)
		return false;
	if (dev1->ifIdProduct != dev2->ifIdProduct)
		return false;

	__USB_FUNC_EXIT__;
	return true;
}

static int check_device_in_device_list(GList *devList, GList *ifList, bool *found)
{
	__USB_FUNC_ENTER__;
	assert(found);
	assert(ifList);

	bool local;
	GList *l;
	UsbInterface *devIf;
	UsbInterface *tmpIf;

	if (!devList) {
		USB_LOG("Device list is empty");
		return 0;
	}

	local = false;
	devIf = (UsbInterface *)(ifList->data);
	for (l = devList ; l ; l = g_list_next(l)) {
		/* If two device have same bus and address,
		 * these devices are same device */
		tmpIf = (UsbInterface *)(l->data);
		if (!find_same_device_interface(devIf, tmpIf))
			continue;

		USB_LOG("Device is found at device list");
		local = true;
		break;
	}

	*found = local;

	__USB_FUNC_EXIT__;
	return 0;
}

static int um_add_device_popup_noti(UmMainData *ad,
			UsbInterface *devIf,
			bool isSupported)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(devIf);

	int ret;
	int i;
	GList *l;
	UsbInterface *tmpIf;

	/* Mounting Mass storage takes time,
	 * so the next procedure is proceeded in the add_mass_storage_cb() */
	if (devIf->ifClass == USB_HOST_MASS_STORAGE)
		return 0;

	/* TODO If scanner and fax are supported by kernel,
	 * These codes should be modified */
	/* If A Usb device has more than two printer interfaces,
	 * only the first popup is displayed */
	i = 0;
	if (devIf->ifClass == USB_HOST_PRINTER) {
		for (l = ad->devList ; l ; l = g_list_next(l)) {
			tmpIf = (UsbInterface *)(l->data);
			if (tmpIf->ifClass != USB_HOST_PRINTER)
				continue;
			if (devIf->ifBus != tmpIf->ifBus)
				continue;
			if (devIf->ifAddress != tmpIf->ifAddress)
				continue;

			i++;
			if (i > 1) {
				__USB_FUNC_EXIT__;
				return 0;
			}
		}
	}

	if (true == isSupported) {
		ret = um_connect_known_device(ad, devIf);
		if (ret < 0) USB_LOG("FAIL: um_connect_known_device()");
	} else { /* false == isSupported */
		USB_LOG("The device is not supported");
		ret = um_connect_unknown_device(ad, devIf);
		if (ret < 0) USB_LOG("FAIL: um_connect_unknown_device()");
	}

	__USB_FUNC_EXIT__;
	return 0;
}

static int um_add_device_to_list(UmMainData *ad, GList *ifList)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(ifList);

	int ret;
	bool isSupported;
	GList *l;
	UsbInterface *devIf;

	for (l = ifList ; l ; l = g_list_next(l)) {
		devIf = (UsbInterface *)(l->data);

		isSupported = is_device_supported(devIf->ifClass);
		ad->devList = g_list_append(ad->devList, devIf);

		ret = um_add_device_popup_noti(ad, devIf, isSupported);
		if (ret < 0) {
			USB_LOG("FAIL: um_add_device_popup_noti()");
			return -1;
		}
	}

	__USB_FUNC_EXIT__;
	return 0;
}

static Eina_Bool um_add_usb_host_device(void *data)
{
	__USB_FUNC_ENTER__;
	assert(data);
	UmMainData *ad = (UmMainData *)data;
	if (ad->addHostTimer) {
		ecore_timer_del(ad->addHostTimer);
		ad->addHostTimer = NULL;
	}

	int ret;
	int i;
	bool found = false;
	libusb_device **list = NULL;
	GList *ifList;

	pm_change_state(LCD_NORMAL);

	show_all_usb_devices(ad->devList);

	ssize_t cnt = libusb_get_device_list(ad->usbctx, &list);
	USB_LOG("The number of USB device: %d", cnt);

	/* Check all of connected USB devices
	 * and find which device is newly connected */
	for (i = 0 ; i < cnt ; i++) {
		ifList = NULL;
		ret = retrieve_interface_info(list[i], &ifList);
		if (ret < 0) {
			USB_LOG("FAIL: retrieve_interface_info()");
			libusb_free_device_list(list, 1);
			return -1;
		}

		found = false;
		/* Check default device list whether or not the device exists */
		ret = check_device_in_device_list(ad->defaultDevList, ifList, &found);
		if (ret < 0) {
			USB_LOG("FAIL: check_device_in_device_list()");
			libusb_free_device_list(list, 1);
			return EINA_FALSE;
		}

		if (found)
			continue;

		/* Check device list whether or not the device exists */
		ret = check_device_in_device_list(ad->devList, ifList, &found);
		if (ret < 0) {
			USB_LOG("FAIL: check_device_in_device_list()");
			libusb_free_device_list(list, 1);
			return EINA_FALSE;
		}

		if (found)
			continue;

		/* 'found == false' means that the device is newly connected
		 * because the device cannot find in the list of devices
		 * which are previously connected */
		ret = um_add_device_to_list(ad, ifList);
		if (ret < 0) {
			USB_LOG("FAIL: um_add_device_to_list()");
			libusb_free_device_list(list, 1);
			return EINA_FALSE;
		}

		show_all_usb_devices(ad->devList);
	}
	libusb_free_device_list(list, 1);

	show_all_usb_devices(ad->devList);

	__USB_FUNC_EXIT__;
	return EINA_TRUE;
}

static int check_and_remove_usb_device(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(ad->devList);
	int j;
	int ret;
	ssize_t cnt;
	bool found;
	GList *l;
	GList *tmpl;
	UsbInterface *devIf;
	libusb_device **list = NULL;
	int devAddr;
	int devBus;

	cnt = libusb_get_device_list(ad->usbctx, &list);
	if (list == NULL) {
		__USB_FUNC_EXIT__;
		return 0;
	}

	USB_LOG("g_list_length(ad->devList): %d, libusb: %d", g_list_length(ad->devList), cnt);
	for (l = g_list_last(ad->devList) ; l ;) {
		found = false;
		for (j = 0 ; j < cnt ; j++) { /* current device list */
			devIf = (UsbInterface *)(l->data);
			devBus = libusb_get_bus_number(list[j]);
			devAddr = libusb_get_device_address(list[j]);

			if (devIf->ifBus != devBus)
				continue;
			if (devIf->ifAddress != devAddr)
				continue;

			found = true;
			break;
		}

		tmpl = l;
		l = g_list_previous(l);
		if (found)
			continue;

		/* Cannot find the device in connected device list
		 * Thus, the device is removed*/
		USB_LOG("found == FALSE");
		ret = um_disconnect_usb_device(ad, (UsbInterface *)(tmpl->data));
		if (ret < 0) {
			USB_LOG("FAIL: um_disconnect_usb_device()");
		}
		ad->devList = g_list_delete_link(ad->devList, tmpl);
	}

	libusb_free_device_list(list, 1);
	__USB_FUNC_EXIT__;
	return 0;
}

static Eina_Bool um_remove_usb_host_device(void *data)
{
	__USB_FUNC_ENTER__;
	assert(data);
	UmMainData *ad = (UmMainData *)data;
	int ret;

	if (ad->rmHostTimer) {
		ecore_timer_del(ad->rmHostTimer);
		ad->rmHostTimer = NULL;
	}
	if (!(ad->devList))
		return EINA_TRUE;

	show_all_usb_devices(ad->devList);

	pm_change_state(LCD_NORMAL);

	ret = check_and_remove_usb_device(ad);
	if (ret < 0) {
		USB_LOG("FAIL: check_removed_usb_device()");
	}

	show_all_usb_devices(ad->devList);

	__USB_FUNC_EXIT__;
	return EINA_TRUE;
}

/* TODO Support unknown USB host devices */
int grant_host_permission(UmMainData *ad, char *appId, int vendor, int product)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(appId);

	UsbInterface *devIf;
	GList *l;

	for (l = ad->devList ; l ; l = g_list_next(l)) {
		devIf = (UsbInterface *)(l->data);

		if (devIf->ifIdVendor != vendor)
			continue;
		if (devIf->ifIdProduct != product)
			continue;

		FREE(devIf->permAppId);
		devIf->permAppId = strdup(appId);
		if (!(devIf->permAppId)) {
			USB_LOG("FAIL: strdup()");
		}
	}
	__USB_FUNC_EXIT__;
	return 0;
}

/* TODO Support unknown USB host devices */
int launch_host_app(char *appId)
{
	__USB_FUNC_ENTER__;
	assert(appId);

	bundle *b;
	int ret;

	b = bundle_create();
	um_retvm_if (!b, -1, "FAIL: bundle_create()");

	ret = aul_launch_app(appId, b);
	bundle_free(b);
	um_retvm_if(0 > ret, -1, "FAIL: aul_launch_app(appId, b)");

	__USB_FUNC_EXIT__;
	return 0;
}

/* TODO Support unknown USB host devices */
Eina_Bool has_host_permission(UmMainData *ad, char *appId, int vendor, int product)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(appId);
	UsbInterface *devIf = NULL;
	GList *l = NULL;
	for (l = ad->devList ; l ; l = g_list_next(l)) {
		devIf = (UsbInterface *)(l->data);

		if (devIf->ifIdVendor != vendor)
			continue;
		if (devIf->ifIdProduct != product)
			continue;

		if (!strncmp(devIf->permAppId, appId, strlen(appId))) {
			__USB_FUNC_EXIT__;
			return EINA_TRUE;
		}
	}

	__USB_FUNC_EXIT__;
	return EINA_FALSE;
}

Eina_Bool is_host_connected(UmMainData *ad, int vendor, int product)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	UsbInterface *devIf;
	GList *l;

	for (l = ad->devList ; l ; l = g_list_next(l)) {
		devIf = (UsbInterface *)(l->data);
		if (devIf->ifIdVendor != vendor)
			continue;
		if (devIf->ifIdProduct != product )
			continue;

		__USB_FUNC_EXIT__;
		return EINA_TRUE;
	}

	__USB_FUNC_EXIT__;
	return EINA_FALSE;
}

void um_uevent_usb_host_added(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	Eina_Bool ret_bool;

	if (ad->addHostTimer) {
		ecore_timer_reset(ad->addHostTimer);
		__USB_FUNC_EXIT__;
		return ;
	}

	ad->addHostTimer = ecore_timer_add(TIMER_WAIT, um_add_usb_host_device, ad);
	if (!(ad->addHostTimer)) {
		USB_LOG("FAIL: ecore_timer_add()");
		ret_bool = um_add_usb_host_device(ad);
		if (ret_bool == EINA_FALSE)
			USB_LOG("um_add_usb_host_device()");
	}

	__USB_FUNC_EXIT__;
}

void um_uevent_usb_host_removed(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	Eina_Bool ret_bool;

	if (ad->rmHostTimer) {
		ecore_timer_reset(ad->rmHostTimer);
		__USB_FUNC_EXIT__;
		return;
	}

	ad->rmHostTimer = ecore_timer_add(TIMER_WAIT, um_remove_usb_host_device, ad);
	if (!(ad->rmHostTimer)) {
		USB_LOG("FAIL: ecore_timer_add()");
		ret_bool = um_remove_usb_host_device(ad);
		if (ret_bool == EINA_FALSE)
			USB_LOG("um_remove_usb_host_device()");
	}

	__USB_FUNC_EXIT__;
}

static void um_load_usbotg_popup(char *popupname, char *popuptype, char *devname)
{
	__USB_FUNC_ENTER__;
	assert(popupname);
	assert(popuptype);

	bundle *b;
	int ret;

	b = bundle_create();
	if (!b) {
		USB_LOG("FAIL: bundle_create()");
		return;
	}

	ret = bundle_add(b, "POPUP_TYPE", popuptype);
	if (ret < 0) {
		USB_LOG("FAIL: bundle_add()");
		bundle_free(b);
		return;
	}

	if (devname) {
		ret = bundle_add(b, "DEVICE_PATH", devname);
		if (ret < 0) {
			USB_LOG("FAIL: bundle_add()");
			bundle_free(b);
			return ;
		}
	}

	if (syspopup_launch(popupname, b) < 0) {
		USB_LOG("FAIL: syspopup_launch()");
	}

	bundle_free(b);

	__USB_FUNC_EXIT__;
}

static void camera_added_popup()
{
	__USB_FUNC_ENTER__;

	um_load_usbotg_popup(USBOTG_SYSPOPUP, CAMERA_ADD, NULL);

	__USB_FUNC_EXIT__ ;
}

bool is_mass_storage_mounted(UmMainData *ad, char *devname)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(devname);

	FILE *file = setmntent(MTAB_FILE, "r");
	struct mntent *mnt_entry;;

	while ((mnt_entry = getmntent(file))) {
		if (!strncmp(mnt_entry->mnt_fsname, devname, strlen(devname))) {
			USB_LOG("%s is mounted", devname);
			__USB_FUNC_EXIT__;
			return true;
		}
	}

	USB_LOG("%s is unmounted", devname);
	__USB_FUNC_EXIT__;
	return false;
}

static void um_show_mounted_mass_storage(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	GList *l = NULL;

	/* Show mounted Mass storage */
	USB_LOG("*******************************");
	USB_LOG("** Mounted Mass Storage List ");
	if (ad->devMSList) {
		for (l = ad->devMSList ; l ; l = g_list_next(l)) {
			USB_LOG("** %s", ((MSDevName *)(l->data))->devName);
		}
	} else {
		USB_LOG("** Mass storage None ");
	}
	USB_LOG("*******************************");

	__USB_FUNC_EXIT__;
}

static int um_ms_list_added(UmMainData *ad, char *devName)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(devName);

	MSDevName *msDevice;

	msDevice = (MSDevName *)malloc(sizeof(MSDevName));
	um_retvm_if(!msDevice, -1, "FAIL: malloc()");

	USB_LOG("devName: %s", devName);
	msDevice->devName = strdup(devName);
	if (!(msDevice->devName)) {
		USB_LOG("FAIL: strdup()");
		FREE(msDevice);
		return -1;
	}

	ad->devMSList = g_list_append(ad->devMSList, msDevice);
	if (!(ad->devMSList)) {
		USB_LOG("FAIL: g_list_append()");
		FREE(msDevice);
		return -1;
	}

	um_show_mounted_mass_storage(ad);

	__USB_FUNC_EXIT__;
	return 0;
}

static int um_ms_list_removed(UmMainData *ad, char *devName)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(devName);
	assert(ad->devMSList);

	GList *l;
	GList *tmpl;
	MSDevName *devNameList;

	for (l = g_list_last(ad->devMSList) ; l ;) {
		tmpl = l;
		l = g_list_previous(l);
		devNameList = (MSDevName *)(tmpl->data);
		if (!strncmp(devNameList->devName, devName, strlen(devName))) {
			ad->devMSList = g_list_delete_link(ad->devMSList, tmpl);
			break;
		}
	}

	if (ad->devMSList) {
		if (0 == g_list_length(ad->devMSList)) {
			g_list_free_full(ad->devMSList, free_ms_list);
			ad->devMSList = NULL;
		}
	}

	um_show_mounted_mass_storage(ad);

	__USB_FUNC_EXIT__;
	return 0;
}

static void um_connect_known_device_mass_storage(UmMainData *ad,
				char *devpath, char *devname, char *fstype)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(devpath);
	assert(devname);
	assert(fstype);

	int ret = -1;
	char *notiTitle = NULL;
	char *notiContent = NULL;

	USB_LOG("USB host mass storage connected");
	ret = um_usb_storage_added(ad, devpath, devname, fstype);
	if (ret != 0) {
		load_connection_popup("USB mass storage mount failed");
		return ;
	}

	notiTitle = "USB mass storage connected";
	notiContent = devname;
	ret = um_append_host_notification(ad, USB_NOTI_MASS_STORAGE_ADDED, devname,
				notiTitle, notiContent, USB_ICON_PATH);
	if (ret < 0) USB_LOG("FAIL: um_append_host_notification()");

	load_connection_popup("IDS_COM_POP_USB_MASS_STORAGE_CONNECTED_ABB2");

	__USB_FUNC_EXIT__;
}

static char *adjust_vendor_name(char *vendor)
{
	__USB_FUNC_ENTER__;
	if (!vendor) return NULL;

	char *str;

	if (!strncasecmp(vendor, VENDOR_HP_1, strlen(VENDOR_HP_1))) {
		__USB_FUNC_EXIT__;
		return strdup(VENDOR_HP_3);
	}

	if (!strncasecmp(vendor, VENDOR_HP_2, strlen(VENDOR_HP_2))) {
		__USB_FUNC_EXIT__;
		return strdup(VENDOR_HP_3);
	}

	if (!strncasecmp(vendor, VENDOR_HP_3, strlen(VENDOR_HP_3))) {
		__USB_FUNC_EXIT__;
		return strdup(VENDOR_HP_3);
	}

	str = vendor;
	*str = toupper(*str);
	while(*(++str)) {
		*str = tolower(*str);
	}

	__USB_FUNC_EXIT__;
	return strdup(vendor);
}

static int cnt_char_of_vendor(char *product, char *vendor)
{
	__USB_FUNC_ENTER__;
	assert(product);
	assert(vendor);

	int cnt;
	char *str;

	str = strcasestr(product, vendor);
	if (!str || str != product) {
		__USB_FUNC_EXIT__;
		return 0;
	}

	cnt = strlen(vendor);
	str = str + cnt;
	while (str) {
		if (*str == ' ' || *str == '_' || *str == '-') {
			cnt++;
			str++;
			continue;
		}
		break;
	}
	USB_LOG("Number of characters of vendor(%s) in product(%s)is %d", vendor, product, cnt);

	__USB_FUNC_EXIT__;
	return cnt;
}

static char *remove_vendor_from_product(char *product, char *vendor)
{
	__USB_FUNC_ENTER__;
	assert(product);
	assert(vendor);

	char *str;
	int cnt;

	cnt = cnt_char_of_vendor(product, vendor);
	str = product + cnt;

	USB_LOG("Product name: %s", str);

	__USB_FUNC_EXIT__;
	return strdup(str);
}

static char *remove_hp_from_product(char *product)
{
	__USB_FUNC_ENTER__;
	assert(product);

	char *str;
	int cnt;

	cnt = cnt_char_of_vendor(product, VENDOR_HP_1);
	if (cnt > 0) {
		str = product + cnt;
		__USB_FUNC_EXIT__;
		return strdup(str);
	}

	cnt = cnt_char_of_vendor(product, VENDOR_HP_2);
	if (cnt > 0) {
		str = product + cnt;
		__USB_FUNC_EXIT__;
		return strdup(str);
	}

	cnt = cnt_char_of_vendor(product, VENDOR_HP_3);
	if (cnt > 0) {
		str = product + cnt;
		__USB_FUNC_EXIT__;
		return strdup(str);
	}

	str = product;

	__USB_FUNC_EXIT__;
	return strdup(product);
}

static char *get_interface_name(UsbInterface *device, char *content)
{
	__USB_FUNC_ENTER__;
	assert(device);
	assert(content);

	char notiContent[BUF_MAX];
	char *product = device->ifIProduct;
	char *vendor = device->ifIManufacturer;

	if (!product)
		product = content;

	if (!vendor) {
		snprintf(notiContent, sizeof(notiContent), "%s", product);
		__USB_FUNC_EXIT__;
		return strdup(notiContent);
	}

	vendor = adjust_vendor_name(vendor);
	if (!vendor)
		vendor = strdup(device->ifIManufacturer);

	if (!strncmp(vendor, VENDOR_HP_3, strlen(VENDOR_HP_3))) {
		product = remove_hp_from_product(product);
	} else {
		product = remove_vendor_from_product(product, vendor);
	}

	if (!product) {
		if (device->ifIProduct)
			product = strdup(device->ifIProduct);
		else
			product = strdup(content);
	}

	snprintf(notiContent, sizeof(notiContent), "%s %s", vendor, product);

	FREE(vendor);
	FREE(product);

	__USB_FUNC_EXIT__;
	return strdup(notiContent);

}

static int um_connect_known_device(UmMainData *ad, UsbInterface *device)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(device);
	int ret = -1;
	char *tickerMsg = NULL;
	char *notiTitle = NULL;
	char *notiContent = NULL;

	switch (device->ifClass) {
	case USB_HOST_HID:
		if (HID_KEYBOARD == device->ifProtocol) {
			USB_LOG("USB host keyboard connected");
			tickerMsg = "IDS_COM_POP_KEYBOARD_CONNECTED_ABB2";
			notiTitle = "Keyboard connected";
			notiContent = get_interface_name(device, DEV_STR_KEYBOARD);
			break;
		}

		if (HID_MOUSE == device->ifProtocol) {
			USB_LOG("USB host mouse connected");
			tickerMsg = "IDS_COM_POP_MOUSE_CONNECTED_ABB2";
			notiTitle = "Mouse connected";
			notiContent = get_interface_name(device, DEV_STR_MOUSE);
			break;
		}

		USB_LOG("USB host unknown HID device connected");
		return 0;

	case USB_HOST_CAMERA:
		USB_LOG("USB host camera connected");
		tickerMsg = "IDS_COM_POP_CAMERA_CONNECTED_ABB2";
		notiTitle = "Camera connected";
		notiContent = get_interface_name(device, DEV_STR_CAMERA);
		camera_added_popup();
		break;
	case USB_HOST_PRINTER:
		USB_LOG("USB host printer connected");
		/* TODO check whether or not this is really printer. (scanner, fax, ... ) */
		tickerMsg = "IDS_COM_POP_PRINTER_CONNECTED_ABB2";
		notiTitle = "Printer connected";
		notiContent = get_interface_name(device, DEV_STR_PRINTER);
		break;
	case USB_HOST_REF_INTERFACE:
	case USB_HOST_MASS_STORAGE:
	case USB_HOST_HUB:
	case USB_HOST_CDC:
	case USB_HOST_MISCELLANEOUS:
	case USB_HOST_VENDOR_SPECIFIC:
	default:
		return 0;
	}

	if (!tickerMsg) {
		USB_LOG("msg of tickernoti is NULL");
		FREE(notiContent);
		__USB_FUNC_EXIT__;
		return 0;
	}

	if (!notiContent)
		notiContent = strdup(DEV_STR_UNSUPPORTED);

	if (notiTitle) {
		ret = um_append_host_notification(ad, USB_NOTI_HOST_ADDED, device,
						notiTitle, notiContent, USB_ICON_PATH);
		if (ret < 0) USB_LOG("FAIL: um_append_host_notification()");
	}

	load_connection_popup(tickerMsg);

	FREE(notiContent);
	__USB_FUNC_EXIT__;
	return 0;
}

static int um_connect_unknown_device(UmMainData *ad, UsbInterface *device)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(device);
	/* TODO Unknown devices should be supported */
	/*
	int ret;

	ret = launch_usb_syspopup(ad, SELECT_PKG_FOR_HOST_POPUP, &device);
	if (ret < 0) {
		USB_LOG("FAIL: launch_usb_syspopup()");
	} */
	__USB_FUNC_EXIT__;
	return 0;
}

static void um_disconnect_usb_device_mass_storage(UmMainData *ad, char *devname)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(devname);
	int ret = -1;
	char *msg = "IDS_COM_POP_USB_MASS_STORAGE_UNEXPECTEDLY_REMOVED";

	USB_LOG("USB host mass storage removed");
	ret = um_usb_storage_removed(ad, devname);
	if (ret < 0) USB_LOG("FAIL: um_usb_storage_removed()");

	ret = um_remove_host_notification(ad, USB_NOTI_MASS_STORAGE_REMOVED, devname);
	if (ret < 0) USB_LOG("FAIL: um_remove_host_notification()");

	load_connection_popup(msg);

	__USB_FUNC_EXIT__;
	return ;
}

static int um_disconnect_usb_device(UmMainData *ad, UsbInterface *devIf)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(devIf);

	int ret;
	char *msg = "IDS_COM_BODY_USB_DEVICE_SAFELY_REMOVED";

	switch (devIf->ifClass) {
	case USB_HOST_MASS_STORAGE:
		USB_LOG("ERROR: This function is not for mass storage");
		return 0;
	case USB_HOST_HUB:
	case USB_HOST_REF_INTERFACE:
	case USB_HOST_CDC:
	case USB_HOST_MISCELLANEOUS:
	case USB_HOST_VENDOR_SPECIFIC:
		__USB_FUNC_EXIT__;
		return 0;
	case USB_HOST_CAMERA:
		um_load_usbotg_popup(USBOTG_SYSPOPUP, CAMERA_REMOVE, NULL);
	case USB_HOST_HID:
	case USB_HOST_PRINTER:
	default:
		USB_LOG("USB device removed~!");
		ret = um_remove_host_notification(ad, USB_NOTI_HOST_REMOVED, devIf);
		if (ret < 0) USB_LOG("FAIL: um_remove_host_notification()");
		break;
	}

	load_connection_popup(msg);
	__USB_FUNC_EXIT__;
	return 0;
}

static inline char *get_devname(char *devpath)
{
	char *r;
	if (!devpath)
		return NULL;

	r = strrchr(devpath, '/');
	if (r)
		return r + 1;
	return devpath;
}

void um_uevent_mass_storage_added(UmMainData *ad, char *devpath, char *fstype)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(devpath);
	assert(fstype);

	int ret;
	char *devname = NULL;

	if (true == is_mass_storage_mounted(ad, devpath)) {
		USB_LOG("The mass storage(%s) is already mounted", devpath);
		return;
	}

	devname = get_devname(devpath);
	USB_LOG("devname: %s", devname);

	um_connect_known_device_mass_storage(ad, devpath, devname, fstype);

	ret = um_ms_list_added(ad, devname);
	if (ret < 0) USB_LOG("FAIL: um_ms_list_added()");

	__USB_FUNC_EXIT__;
}

void um_uevent_mass_storage_removed(UmMainData *ad, char *devpath)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(devpath);
	int ret;
	char *devname = NULL;

	if (false == is_mass_storage_mounted(ad, devpath)) {
		USB_LOG("The mass storage(%s) is already unmounted", devpath);
		return ;
	}

	devname = get_devname(devpath);
	USB_LOG("devname: %s", devname);

	um_disconnect_usb_device_mass_storage(ad, devname);

	ret = um_ms_list_removed(ad, devname);
	if (ret < 0) USB_LOG("FAIL: um_ms_list_removed()");

	__USB_FUNC_EXIT__;
}

void disconnect_usb_host(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	um_release_all_device(ad);
	__USB_FUNC_EXIT__;
}

/*************************************************************/
/* Mounting Mass Storage (USB storage)                       */
/*************************************************************/
static int um_mount_device(char *devpath, char *devname, char *fstype)
{
	__USB_FUNC_ENTER__;
	assert(devpath);
	assert(devname);
	assert(fstype);

	if (access(devpath, F_OK) != 0) {
		USB_LOG("Failed to find device: DEVICE(%s)", devpath);
		return -1;
	}

	int r = -1;
	char buf_mnt_point[BUF_MAX];

	snprintf(buf_mnt_point, BUF_MAX, "%s/%s", MOUNT_POINT, devname);

	/* Make directory to mount */
	r = mkdir(buf_mnt_point, 0755);
	if (r < 0) {
		if (errno == EEXIST) {
			USB_LOG("Directory is already exsited: %s", buf_mnt_point);
		} else {
			USB_LOG("FAIL: Make Directory Failed: %s", buf_mnt_point);
			return -1;
		}
	}

	/* TODO Support mounting other file systems */
	/* Mount block device on mount point */
	if (!strncmp(fstype, "vfat", strlen(fstype))) {
		r = mount(devpath, buf_mnt_point, "vfat", 0, "uid=0,gid=0,dmask=0000,fmask=0111,iocharset=iso8859-1,utf8,shortname=mixed,smackfsroot=*,smackfsdef=*");
	} else {
		USB_LOG("ERROR: File system type (%s) is not supported", fstype);
		return -1;
	}

	if (r < 0) {
		r = rmdir(buf_mnt_point);
		USB_LOG("FAIL: Mount failed: MOUNT PATH(%s)", buf_mnt_point);
		return -1;
	}
	USB_LOG("Mount Complete: MOUNT PATH(%s)", buf_mnt_point);

	__USB_FUNC_EXIT__;
	return 0;
}

static int um_unmount_device(char *mnt_point)
{
	__USB_FUNC_ENTER__;
	assert(mnt_point);
	if (access(mnt_point, F_OK) != 0) {
		USB_LOG("Failed to find path: MOUNT PATH(%s)", mnt_point);
		return -1;
	}

	int ret = -1;

	/* Umount block device */
	ret = umount2(mnt_point, MNT_DETACH);
	if (ret < 0) {
		USB_LOG("Unmounting is unabled: MOUNT PATH(%s)", mnt_point);
		ret = rmdir(mnt_point);
		if (ret < 0) {
			USB_LOG("Removing Directory is unabled: PATH(%s)", mnt_point);
		}
		return -1;
	}

	um_load_usbotg_popup(USBOTG_SYSPOPUP, STORAGE_REMOVE, mnt_point);

	/* Clean up unmounted directory */
	ret = rmdir(mnt_point);
	if (ret < 0) {
		USB_LOG("FAIL: Removing Directory is unabled: PATH(%s)", mnt_point);
	}
	USB_LOG("Unmount/Remove Complete: MOUNT PATH(%s)", mnt_point);

	__USB_FUNC_EXIT__;
	return 0;
}

static int um_usb_storage_mount_tmpfs(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	assert(ad);

	if (ad->tmpfsMounted) {
		USB_LOG("tmpfs is already mounted");
		return 0;
	}

	/* Mount tmpfs for protecting user data */
	if (mount(FS_TMPFS, MOUNT_POINT, FS_TMPFS, 0, "") < 0) {
		USB_LOG("FAIL: mount(%s)", MOUNT_POINT);
		return -1;
	}

	/* Change permission to avoid to write user data on tmpfs */
	if (chmod(MOUNT_POINT, 0755) < 0) {
		USB_LOG("FAIL: Failed to change mode: %s", MOUNT_POINT);
		if (umount2(MOUNT_POINT, MNT_DETACH) < 0) USB_LOG("FAIL: umount2()");
		return -1;
	}
	ad->tmpfsMounted = true;

	__USB_FUNC_EXIT__;
	return 0;
}

static int um_usb_storage_added(UmMainData *ad, char *devpath, char *devname, char *fstype)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(devpath);
	assert(devname);
	assert(fstype);

	char *mounted_check = NULL;
	char buf_mnt_point[BUF_MAX];
	FILE *file = NULL;
	struct mntent *mnt_entry = NULL;

	/* Check whether mount point directory is exist */
	if (access(MOUNT_POINT, F_OK) < 0) {
		if (mkdir(MOUNT_POINT, 0755) < 0) {
			USB_LOG("FAIL: Make Mount Directory Failed: %s", MOUNT_POINT);
			return -1;
		}
	}

	if (um_usb_storage_mount_tmpfs(ad) < 0) {
		USB_LOG("FAIL: um_usb_storage_mount_tmpfs()");
		return -1;
	}

	snprintf(buf_mnt_point, BUF_MAX, "%s/%s", MOUNT_POINT, devname);

	if (um_mount_device(devpath, devname, fstype)) {
		USB_LOG("FAIL: um_mount_device()");
	}

	/* Check whether block deivce is mounted */
	file = setmntent(MTAB_FILE, "r");
	while ((mnt_entry = getmntent(file))) {
		mounted_check = strstr(mnt_entry->mnt_fsname, devpath);
		if (mounted_check) break;
	}
	endmntent(file);
	if (!mounted_check) {
		USB_LOG("Nothing to be mounted: DEVICE(%s)", devname);
		return -1;
	}

	/* Broadcast mounting notification */
	if (vconf_set_int(VCONFKEY_SYSMAN_ADDED_USB_STORAGE, 1) < 0) {
		USB_LOG("FAIL: vconf_set_int(%s)", VCONFKEY_SYSMAN_ADDED_USB_STORAGE);
		return -1;
	}

	um_load_usbotg_popup(USBOTG_SYSPOPUP, STORAGE_ADD, buf_mnt_point);

	__USB_FUNC_EXIT__;
	return 0;
}

static int um_usb_storage_removed(UmMainData *ad, char *devname)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(devname);
	char buf_mnt_point[BUF_MAX];

	snprintf(buf_mnt_point, BUF_MAX, "%s/%s", MOUNT_POINT, devname);
	USB_LOG("buf_mnt_point: %s", buf_mnt_point);

	if (um_unmount_device(buf_mnt_point) != 0) {
		USB_LOG("FAIL: um_unmount_device()");
		return -1;
	}

	if (ad->devMSList && g_list_length(ad->devMSList) <= 1) {
		if (umount2(MOUNT_POINT, MNT_DETACH) == 0) {
			USB_LOG("tmpfs is unmounted");
			ad->tmpfsMounted = false;
		} else {
			USB_LOG("FAIL: umount2(%s)", MOUNT_POINT);
		}
	}

	if (vconf_set_int(VCONFKEY_SYSMAN_REMOVED_USB_STORAGE, 1) < 0) {
		USB_LOG("FAIL: vconf_set_int(%s)", VCONFKEY_SYSMAN_REMOVED_USB_STORAGE);
	}
	__USB_FUNC_EXIT__;
	return 0;
}
