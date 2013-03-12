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

#include <vconf.h>
#include <dirent.h>
#include "um_usb_host_manager.h"

int tmpfs_mounted = 0;
int added_noti_value = 0;
int removed_noti_value = 0;

void free_func(gpointer data)
{
	__USB_FUNC_ENTER__;
	UsbHost *device = (UsbHost *)data;
	FREE(device->permittedAppId);
	__USB_FUNC_EXIT__;
}

int retrieve_device_info(libusb_device *device,
						int *deviceClass,
						int *deviceSubClass,
						int *deviceProtocol,
						int *idVendor,
						int *idProduct,
						int *bus,
						int *devAddress)
{
	__USB_FUNC_ENTER__;
	if (device == NULL) return -1;
	struct libusb_device_descriptor desc;
	int ret = -1;
	libusb_device_handle *handle = NULL;
	ret = libusb_get_device_descriptor(device, &desc);
	if (ret != 0) {
		USB_LOG("FAIL: libusb_get_device_descriptor()\n");
		return -1;
	}

	/* If device class of device descriptor is 0,
	 * the real device class information would be located in the interface descriptor */
	if (0 == desc.bDeviceClass) {
		struct libusb_config_descriptor *config = NULL;
		ret = libusb_get_active_config_descriptor(device, &config);
		if (ret != 0) {
			USB_LOG("FAIL: libusb_get_active_config_descriptor(device, &config)");
			libusb_free_config_descriptor(config);
			return -1;
		}
		*deviceClass = config->interface->altsetting[0].bInterfaceClass;
		*deviceSubClass = config->interface->altsetting[0].bInterfaceSubClass;
		*deviceProtocol = config->interface->altsetting[0].bInterfaceProtocol;
		libusb_free_config_descriptor(config);
	} else {
		*deviceClass = desc.bDeviceClass;
		*deviceSubClass = desc.bDeviceSubClass;
		*deviceProtocol = desc.bDeviceProtocol;
	}
	*idVendor = desc.idVendor;
	*idProduct = desc.idProduct;
	*bus = libusb_get_bus_number(device);
	*devAddress = libusb_get_device_address(device);

	__USB_FUNC_EXIT__;
	return 0;
}

int umReleaseAllDevice(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	if (ad->devList) {
		int cnt = g_list_length(ad->devList);
		int i;
		int ret;
		for ( i = cnt-1 ; i >= 0 ; i--) {
			ret = um_disconnect_usb_device(ad,
							((UsbHost *)(g_list_nth_data(ad->devList, i)))->deviceClass);
			if (ret < 0) USB_LOG("FAIL: um_disconnect_usb_device()");
			ad->devList = g_list_delete_link(ad->devList, g_list_nth(ad->devList, i));
		}

		if (ad->devList) {
			g_list_free_full(ad->devList, free_func);
			ad->devList = NULL;
		}
	}
	load_connection_popup(ad, "IDS_COM_POP_USB_CONNECTOR_DISCONNECTED", TICKERNOTI_ORIENTATION_TOP);

	__USB_FUNC_EXIT__;
	return 0;
}

int compareTwoDevices(libusb_device *usbDevice, UsbHost *listedDevice)
{
	__USB_FUNC_ENTER__;
	if (usbDevice == NULL || listedDevice == NULL) return -1;
	int ret = -1;
	int deviceClass;
	int deviceSubClass;
	int deviceProtocol;
	int idVendor;
	int idProduct;
	int bus;
	int devAddress;
	ret = retrieve_device_info(usbDevice,
							&deviceClass,
							&deviceSubClass,
							&deviceProtocol,
							&idVendor,
							&idProduct,
							&bus,
							&devAddress);
	if (ret < 0) {
		USB_LOG("FAIL: retrieve_device_info()\n");
		return -1;
	}
	if (listedDevice->deviceClass == deviceClass
		&& listedDevice->deviceSubClass == deviceSubClass
		&& listedDevice->deviceProtocol == deviceProtocol
		&& listedDevice->idVendor == idVendor
		&& listedDevice->idProduct == idProduct
		&& listedDevice->bus == bus
		&& listedDevice->deviceAddress == devAddress)
	{
		USB_LOG("Two devices are same\n");
		__USB_FUNC_EXIT__;
		return 0;
	} else {
		USB_LOG("Two devices are different\n");
		__USB_FUNC_EXIT__;
		return 1;
	}
}

bool is_device_driver_active(libusb_device *device, int devClass)
{
	__USB_FUNC_ENTER__;
	if (!device) return false;
	int ret = -1;
	bool result;
	libusb_device_handle *handle;

	switch (devClass) {
	case USB_HOST_CAMERA:
		/* Camera is supported by libgphpoto2, not by kernel
		 * The directory is made When camera is connected, and removed when camera is removed */
		if (0 == access("/tmp/camera", F_OK)) {
			result = true;
		} else {
			result = false;
		}
		break;
	case USB_HOST_CDC:
	case USB_HOST_HID:
	case USB_HOST_PRINTER:
	case USB_HOST_MASS_STORAGE:
	case USB_HOST_HUB:
		ret = libusb_open(device, &handle);
		if (ret != 0) {
			USB_LOG("FAIL: libusb_open()");
			return false;
		}

		/* We can check the usb device is available by checking if kernel driver is actice or not */
		ret = libusb_kernel_driver_active(handle, 0);
		if (ret == 1) {
			USB_LOG("Device driver is active");
			result = true;
		} else if (ret == 0) {
			USB_LOG("No device driver is active");
			result = false;
		} else {
			USB_LOG("FAIL: libusb_kernel_driver_active()");
			result = false;
		}
		libusb_close(handle);
		break;
	default:
		USB_LOG("The device(class: %d) is not supported by kernel", devClass);
		result = false;
		break;
	}
	__USB_FUNC_EXIT__;
	return result;
}

int add_usb_device_to_list(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	int ret = -1;
	int i, j;
	int deviceClass;
	int deviceSubClass;
	int deviceProtocol;
	int idVendor;
	int idProduct;
	int bus;
	int devAddress;
	bool found = false;
	libusb_device **list = NULL;
	pm_change_state(LCD_NORMAL);

	ssize_t cnt = libusb_get_device_list(ad->usbctx, &list);

	for (i = 0 ; i < cnt ; i++) {
		ret = retrieve_device_info(list[i],
								&deviceClass,
								&deviceSubClass,
								&deviceProtocol,
								&idVendor,
								&idProduct,
								&bus,
								&devAddress);
		if (ret < 0) {
			USB_LOG("FAIL: retrieve_device_info()\n");
			libusb_free_device_list(list, 1);
			return -1;
		}
		USB_LOG("Device class: %d", deviceClass);

		/* Check default device list */
		if (ad->defaultDevList) {
			for (j = 0 ; j < g_list_length(ad->defaultDevList) ; j++) {
				if (0 == compareTwoDevices(list[i],
							((UsbHost *)(g_list_nth_data(ad->defaultDevList, j))))) {
					found = true;
					break;
				}
			}
			if (found == true) USB_LOG("Device is found at default device list");
		}

		/* Check device list */
		if (found == false && ad->devList) {
			for (j = 0 ; j < g_list_length(ad->devList) ; j++) {
				if (0 == compareTwoDevices(list[i],
							((UsbHost *)(g_list_nth_data(ad->devList, j))))) {
					found = true;
					break;
				}
			}
			if (found == true) USB_LOG("Device is found at device list");
		}

		if (found == false) {
			UsbHost *device = (UsbHost *)malloc(sizeof(UsbHost));
			device->permittedAppId = NULL;
			device->deviceClass = deviceClass;
			device->deviceSubClass = deviceSubClass;
			device->deviceProtocol = deviceProtocol;
			device->idVendor = idVendor;
			device->idProduct = idProduct;
			device->bus = bus;
			device->deviceAddress = devAddress;
			ad->devList = g_list_append(ad->devList, device);

			if (true == is_device_supported(deviceClass)) {
				if (deviceClass != USB_HOST_MASS_STORAGE) {
					j = 0;
					while(false == is_device_driver_active(list[i], deviceClass)) {
						USB_LOG("Wait...");
						usleep(100000);
						j++;
						if (j > 100) break;	/* Try for 10 seconds */
					}
					if (j <= 100) {
						ret = um_connect_known_device(ad, deviceClass,
														deviceSubClass, deviceProtocol);
						if (ret < 0) USB_LOG("FAIL: um_connect_known_device()");
					}
				}
			} else {
				ret = um_connect_unknown_device(ad, deviceClass, deviceSubClass,
											deviceProtocol, idVendor, idProduct);
				if (ret < 0) USB_LOG("FAIL: um_connect_unknown_device()");
			}
		} else {
			found = false;
		}
	}
	libusb_free_device_list(list, 1);

	__USB_FUNC_EXIT__;
	return 0;
}

int remove_usb_device_to_list(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	if (!(ad->devList)) return 0;
	int i, j;
	int ret = -1;
	int recheck = 0;
	ssize_t cnt;
	bool found = false;
	libusb_device **list = NULL;
	pm_change_state(LCD_NORMAL);

	/* When getting the device list using libusb apis,
	 * the device list would not be updated.
	 * So, we should re-check when we cannot find device removed */
	do {
		cnt = libusb_get_device_list(ad->usbctx, &list);
		if (list == NULL) {
			/* USB device status is not changed */
			__USB_FUNC_EXIT__;
			return 0;
		}
		for (i = 0 ; i < g_list_length(ad->devList) ; i++) {
			for (j = 0 ; j < cnt ; j++) {
				if ( 0 == compareTwoDevices(list[j],
						 ((UsbHost *)(g_list_nth_data(ad->devList, i)))) ) {
					found = true;
					break;
				}
			}
			if (found == true) {
				found = false;
			} else {
				ret = um_disconnect_usb_device(ad,
						((UsbHost *)(g_list_nth_data(ad->devList, i)))->deviceClass);
				if (ret < 0) {
					USB_LOG("FAIL: um_disconnect_usb_device()");
				}
				ad->devList = g_list_delete_link(ad->devList, g_list_nth(ad->devList, i));
				found = true;
				break;
			}
		}
		recheck++;
		libusb_free_device_list(list, 1);
		USB_LOG("Wait...");
		sleep(1);	/* Wait for 1 sec to get valid libusb device list */
	} while (found == false && recheck < 3);	/* Try 3 times */

	__USB_FUNC_EXIT__;
	return 0;
}

int grantHostPermission(UmMainData *ad, char *appId, int vendor, int product)
{
	__USB_FUNC_ENTER__;
	if (!ad || !appId) return -1;
	if (ad->permittedPkgForAcc != NULL) {
		USB_LOG("Previous permitted pkg is removed\n");
	}

	int i;
	for (i = 0; i < g_list_length(ad->devList) ; i++) {
		if ( ((UsbHost *)(g_list_nth_data(ad->devList, i)))->idVendor == vendor
					&& ((UsbHost *)(g_list_nth_data(ad->devList, i)))->idProduct == product) {
			FREE(((UsbHost *)(g_list_nth_data(ad->devList, i)))->permittedAppId);
			((UsbHost *)(g_list_nth_data(ad->devList, i)))->permittedAppId = strdup(appId);
			USB_LOG("Permitted app for the device is %s\n", ((UsbHost *)(g_list_nth_data(ad->devList, i)))->permittedAppId);
			break;
		}
	}
	__USB_FUNC_EXIT__;
	return 0;
}

int launch_host_app(char *appId)
{
	__USB_FUNC_ENTER__;
	if (appId == NULL) return -1;
	bundle *b = NULL;
	b = bundle_create();
	um_retvm_if(!b, -1, "FAIL: bundle_create()");
	int ret = aul_launch_app(appId, b);
	bundle_free(b);
	um_retvm_if(0 > ret, -1, "FAIL: aul_launch_app(appId, b)");
	__USB_FUNC_EXIT__;
	return 0;
}

Eina_Bool hasHostPermission(UmMainData *ad, char *appId, int vendor, int product)
{
	__USB_FUNC_ENTER__;
	if (!ad || !appId) return EINA_FALSE;
	int i;
	for (i = 0; i < g_list_length(ad->devList) ; i++) {
		if ( ((UsbHost *)(g_list_nth_data(ad->devList, i)))->idVendor == vendor
					&& ((UsbHost *)(g_list_nth_data(ad->devList, i)))->idProduct == product ) {
			if (0 == strncmp(((UsbHost *)(g_list_nth_data(ad->devList, i)))->permittedAppId,
							appId, strlen(appId))) {
				__USB_FUNC_EXIT__;
				return EINA_TRUE;
			} else {
				break;
			}
		}
	}
	__USB_FUNC_EXIT__;
	return EINA_FALSE;
}

Eina_Bool is_host_connected(UmMainData *ad, int vendor, int product)
{
	__USB_FUNC_ENTER__;
	if (!ad) return EINA_FALSE;
	int i;
	for (i = 0 ; i < g_list_length(ad->devList) ; i++) {
		if ( ((UsbHost *)(g_list_nth_data(ad->devList, i)))->idVendor == vendor
					&& ((UsbHost *)(g_list_nth_data(ad->devList, i)))->idProduct == product ) {
			__USB_FUNC_EXIT__;
			return EINA_TRUE;
		}
	}
	__USB_FUNC_EXIT__;
	return EINA_FALSE;
}

int show_all_usb_devices(GList *devList, int option)
{
	__USB_FUNC_ENTER__;
	if (!(devList)) {
		return 0;
	}
	if (option == 1) {
		int i;
		USB_LOG("***********************************\n");
		USB_LOG("** USB devices list \n");
		USB_LOG("** \n");
		for (i = 0 ; i < g_list_length(devList) ; i++) {
			USB_LOG("** Device number: %d\n", i);
			USB_LOG("** Permitted app : %s\n", ((UsbHost *)(g_list_nth_data(devList, i)))->permittedAppId);
			USB_LOG("** Device Class : %u\n", ((UsbHost *)(g_list_nth_data(devList, i)))->deviceClass);
			USB_LOG("** Device Subclass : %u\n", ((UsbHost *)(g_list_nth_data(devList, i)))->deviceSubClass);
			USB_LOG("** Device Protocol : %u\n", ((UsbHost *)(g_list_nth_data(devList, i)))->deviceProtocol);
			USB_LOG("** Device Vendor ID : %u\n", ((UsbHost *)(g_list_nth_data(devList, i)))->idVendor);
			USB_LOG("** Device Product ID: %u\n", ((UsbHost *)(g_list_nth_data(devList, i)))->idProduct);
			USB_LOG("** Device Bus number: %u\n", ((UsbHost *)(g_list_nth_data(devList, i)))->bus);
			USB_LOG("** Device Address: %u\n", ((UsbHost *)(g_list_nth_data(devList, i)))->deviceAddress);
			USB_LOG("*************************************** ");
		}
	} else {
		USB_LOG("FAIL: Option is not correct\n");
	}
	__USB_FUNC_EXIT__;
	return 0;
}

void add_host_noti_cb(void *data)
{
	__USB_FUNC_ENTER__;
	UmMainData *ad = (UmMainData *)data;
	int ret = show_all_usb_devices(ad->devList,1);
	if (ret != 0) USB_LOG("FAIL: show_all_usb_devices(ad)\n");

	ret = add_usb_device_to_list(ad);
	um_retm_if(0 != ret, "FAIL: add_usb_device_to_list(ad)");

	ret = show_all_usb_devices(ad->devList,1);
	if (ret != 0) USB_LOG("FAIL: show_all_usb_devices(ad)\n");
	__USB_FUNC_EXIT__;
}

void remove_host_noti_cb(void *data)
{
	__USB_FUNC_ENTER__;
	UmMainData *ad = (UmMainData *)data;
	int ret = show_all_usb_devices(ad->devList,1);
	if (ret != 0) USB_LOG("FAIL: show_all_usb_devices(ad)\n");

	ret = remove_usb_device_to_list(ad);
	um_retm_if(0 != ret, "FAIL: remove_usb_device_to_list(ad)");

	ret = show_all_usb_devices(ad->devList,1);
	if (ret != 0) USB_LOG("FAIL: show_all_usb_devices(ad)\n");
	__USB_FUNC_EXIT__;
}

static void camera_added_popup()
{
	__USB_FUNC_ENTER__;
	int ret = -1;
	bundle *b = NULL;

	DIR *dp;
	struct dirent *dir;
	struct stat stat;
	char buf[255] = "unknown_camera";

	if ((dp = opendir("/tmp/camera")) == NULL) {
		USB_LOG("Can not open directory");
		return ;
	}
	chdir("/tmp/camera");

	while (dir = readdir(dp)) {
		memset(&stat, 0, sizeof(struct stat));
		lstat(dir->d_name, &stat);
		if (S_ISDIR(stat.st_mode) || S_ISLNK(stat.st_mode)) {
			if (strncmp(".", dir->d_name, 1) == 0
					|| strncmp("..", dir->d_name, 2) == 0)
				continue;
			snprintf(buf, 255, "%s", dir->d_name);
		}
	}
	closedir(dp);

	b = bundle_create();
	bundle_add(b, "_SYSPOPUP_CONTENT_", "camera_add");
	bundle_add(b, "device_name", buf);
	ret = syspopup_launch("usbotg-syspopup", b);
	USB_LOG("ret: %d", ret);
	if (ret < 0) {
		USB_LOG("FAIL: syspopup_launch(usbotg-syspopup, b)");
	}
	bundle_free(b);

	__USB_FUNC_EXIT__ ;
	return ;
}

static bool is_mass_storage_mounted(UmMainData *ad, int event)
{
	__USB_FUNC_ENTER__;
	if (!ad) return false;
	int ret;
	char *storage_path = NULL;
	char *storage = NULL;
	char storage_mount_path[BUF_MAX];
	if (event == 0) {	/* 'event == 0' means that mass storage is just connected */
		storage_path = vconf_get_str(VCONFKEY_SYSMAN_ADDED_STORAGE_UEVENT);
		if (storage_path != NULL) {
			storage = strrchr(storage_path, '/');
			storage++;	/* remove '/' */
			USB_LOG("storage: %s", storage);
		}
	} else if (event == 1) {	/*  'event == 1' means that mass storage is just removed */
		storage = vconf_get_str(VCONFKEY_SYSMAN_REMOVED_STORAGE_UEVENT);
	}
	if (storage) {
		snprintf(storage_mount_path, BUF_MAX, "%s/%s", MOUNT_POINT, storage);
		USB_LOG("storage_mount_path: %s", storage_mount_path);
		ret = access(storage_mount_path, F_OK);
		if (ret == 0) {
			USB_LOG("This mass storage is mounted");
			__USB_FUNC_EXIT__;
			return true;
		} else {
			USB_LOG("This mass storage is unmounted");
			__USB_FUNC_EXIT__;
			return false;
		}
	}
	__USB_FUNC_EXIT__;
	return false;
}

int um_connect_known_device(UmMainData *ad, int class, int subClass, int protocol)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;
	char *msg = NULL;
	int i;

	switch (class) {
	case USB_HOST_HID:
		if (HID_KEYBOARD == protocol) {
			USB_LOG("USB host keyboard connected");
			msg = "IDS_COM_POP_KEYBOARD_CONNECTED_ABB2";
		} else if (HID_MOUSE == protocol) {
			USB_LOG("USB host mouse connected");
			msg = "IDS_COM_POP_MOUSE_CONNECTED_ABB2";
		} else {
			USB_LOG("USB host unknown HID device connected");
			msg = "IDS_BT_POP_CONNECTED_TO_HID_DEVICE";
		}
		break;
	case USB_HOST_CAMERA:
		USB_LOG("USB host camera connected");
		msg = "IDS_COM_POP_CAMERA_CONNECTED_ABB2";
		camera_added_popup();
		break;
	case USB_HOST_PRINTER:
		USB_LOG("USB host printer connected");
		msg = "IDS_COM_POP_PRINTER_CONNECTED_ABB2";
		/* We should add some operations for printer */
		break;
	case USB_HOST_MASS_STORAGE:
		USB_LOG("USB host mass storage connected");
		ret = um_usb_storage_added();
		if (0 > ret) {
			msg = "USB mass storage mount failed";
		} else {
			msg = "IDS_COM_POP_USB_MASS_STORAGE_CONNECTED_ABB2";
		}
		break;
	case USB_HOST_HUB:
		USB_LOG("USB host hub connected");
		break;
	default:
		USB_LOG("USB host unknown device connected");
		break;
	}
	if (msg == NULL) {
		USB_LOG("msg of tickernoti is NULL");
		__USB_FUNC_EXIT__;
		return 0;
	}

	load_connection_popup(ad, msg, TICKERNOTI_ORIENTATION_TOP);
	__USB_FUNC_EXIT__;
	return 0;
}

int um_connect_unknown_device(UmMainData *ad,
							int class,
							int subClass,
							int protocol,
							int vendor,
							int product)
{
	__USB_FUNC_ENTER__;
	int ret = load_system_popup_with_deviceinfo(ad, SELECT_PKG_FOR_HOST_POPUP,
										class, subClass, protocol, vendor, product);
	if (ret < 0) {
		USB_LOG("FAIL: load_system_popup_with_deviceinfo()");
	}
	__USB_FUNC_EXIT__;
	return 0;
}

int um_disconnect_usb_device(UmMainData *ad, int class)
{
	__USB_FUNC_ENTER__;
	if(!ad) return -1;
	int ret = -1;
	char *msg = "IDS_COM_BODY_USB_DEVICE_SAFELY_REMOVED";
	int i;

	switch (class) {
	case USB_HOST_MASS_STORAGE:
		USB_LOG("USB host mass storage removed");
		if (false == is_mass_storage_mounted(ad, 1)) {
			USB_LOG("Mass storage is already unmounted");
			__USB_FUNC_EXIT__;
			return 0;
		}
		msg = "IDS_COM_POP_USB_MASS_STORAGE_UNEXPECTEDLY_REMOVED";
		ret = um_usb_storage_removed();
		if (ret < 0) USB_LOG("FAIL: um_usb_storage_removed()");
		break;
	case USB_HOST_HUB:
		USB_LOG("USB hub removed");
		__USB_FUNC_EXIT__;
		return 0;
	case USB_HOST_HID:
	case USB_HOST_CAMERA:
	case USB_HOST_PRINTER:
	default:
		break;
	}

	load_connection_popup(ad, msg, TICKERNOTI_ORIENTATION_TOP);
	__USB_FUNC_EXIT__;
	return 0;
}

void add_mass_storage_cb(keynode_t *in_key, void *data)
{
	__USB_FUNC_ENTER__;
	if (!data) return ;
	UmMainData *ad = (UmMainData *)data;
	int ret;
	if (false == is_mass_storage_mounted(ad, 0)) {
		ret = um_connect_known_device(ad, USB_HOST_MASS_STORAGE, 0, 0);
		if (ret < 0) USB_LOG("FAIL: um_connect_known_device()");
	}
	__USB_FUNC_EXIT__;
}

void remove_mass_storage_cb(keynode_t *in_key, void *data)
{
	__USB_FUNC_ENTER__;
	if (!data) return ;
	UmMainData *ad = (UmMainData *)data;
	if (true == is_mass_storage_mounted(ad, 1)) {
		int ret = um_disconnect_usb_device(ad, USB_HOST_MASS_STORAGE);
		if (ret < 0) USB_LOG("FAIL: um_disconnect_usb_device()");
	}
	__USB_FUNC_EXIT__;
}

int disconnectUsbHost(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	int ret = umReleaseAllDevice(ad);
	if (ret < 0) USB_LOG("FAIL: umReleaseAllDevice(ad)");
	__USB_FUNC_EXIT__;
	return 0;
}

/*************************************************************/
/* Mounting Mass Storage (USB storage)                       */
/*************************************************************/
static int um_mount_device(char *dev)
{
	__USB_FUNC_ENTER__;
	if (access(dev, F_OK) != 0) {
		USB_LOG("Failed to find device: DEVICE(%s)", dev);
		return -1;
	}

	int fd = -1;
	int r = -1;
	char *rel_mnt_point;
	char buf_mnt_point[BUF_MAX];

	rel_mnt_point = strrchr(dev, '/');
	if (rel_mnt_point == NULL) {
		USB_LOG("Get Relative Mount Path Failed");
		return -1;
	}

	snprintf(buf_mnt_point, BUF_MAX, "%s%s", MOUNT_POINT, rel_mnt_point);

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

	/* Mount block device on mount point */
	r = mount(dev, buf_mnt_point, "vfat", 0, "uid=0,gid=0,dmask=0000,fmask=0111,iocharset=iso8859-1,utf8,shortname=mixed,smackfsroot=*,smackfsdef=*");
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
	if (mnt_point == NULL) return -1;
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

	bundle *b = NULL;
	b = bundle_create();
	bundle_add(b, "_SYSPOPUP_CONTENT_", "otg_remove");
	ret = syspopup_launch("usbotg-syspopup", b);
	if (ret < 0) {
		USB_LOG("FAIL: popup lauch failed\n");
	}
	bundle_free(b);

	/* Clean up unmounted directory */
	ret = rmdir(mnt_point);
	if (ret < 0) {
		USB_LOG("FAIL: Removing Directory is unabled: PATH(%s)", mnt_point);
	}
	USB_LOG("Unmount/Remove Complete: MOUNT PATH(%s)", mnt_point);

	__USB_FUNC_EXIT__;
	return 0;
}

static int um_usb_storage_added()
{
	__USB_FUNC_ENTER__;
	int ret = -1;
	int fd = -1;
	int i;

	char *buf_dev;
	char *mounted_check;
	char *rel_mnt_point;
	char buf_mnt_point[BUF_MAX];
	char *sd_char;

	do {
		i = 0;
		buf_dev = vconf_get_str(VCONFKEY_SYSMAN_ADDED_STORAGE_UEVENT);
		if (buf_dev != NULL) {
			sd_char = strstr(buf_dev, "sd");
			if (sd_char == NULL) buf_dev = NULL;
		}
		USB_LOG("Wait...");
		sleep(1);
		i++;
		if (i == 5) break;
	} while(buf_dev == NULL);


	/* Check whether mount point directory is exist */
	if (access(MOUNT_POINT, F_OK) < 0) {
		if (mkdir(MOUNT_POINT, 0755) < 0) {
			USB_LOG("FAIL: Make Mount Directory Failed: %s", MOUNT_POINT);
			return -1;
		}
	}

	/* Mount tmpfs for protecting user data */
	if (tmpfs_mounted != 1) {
		if (mount("tmpfs", MOUNT_POINT, "tmpfs", 0, "") < 0) {
			if (errno != EBUSY) {
				USB_LOG("FAIL: Failed to mount USB Storage Mount Directory: %s", MOUNT_POINT);
				return -1;
			}
		} else {
			/* Change permission to avoid to write user data on tmpfs */
			if (chmod(MOUNT_POINT, 0755) < 0) {
				USB_LOG("FAIL: Failed to change mode: %s", MOUNT_POINT);
				umount2(MOUNT_POINT, MNT_DETACH);
				return -1;
			}
			tmpfs_mounted = 1;
		}
	}

	USB_LOG("buf_dev: %s", buf_dev);
	USB_LOG("strrchr(buf_dev): %s", strrchr(buf_dev, '/'));
	rel_mnt_point = strrchr(buf_dev, '/');

	if (rel_mnt_point == NULL) {
		USB_LOG("FAIL: Get Relative Mount Path Failed");
		return -1;
	}
	snprintf(buf_mnt_point, BUF_MAX, "%s%s", MOUNT_POINT, rel_mnt_point);

	if (um_mount_device(buf_dev)) {
		USB_LOG("FAIL: um_mount_device(%s)", buf_dev);
	}

	FILE *file = setmntent(MTAB_FILE, "r");
	struct mntent *mnt_entry;

	/* Check whether block deivce is mounted */
	while (mnt_entry = getmntent(file)) {
		mounted_check = strstr(mnt_entry->mnt_fsname, buf_dev);
		if (mounted_check != NULL) {
			if (added_noti_value < INT_MAX) {
				++added_noti_value;
			} else {
				added_noti_value = 1;
			}
			/* Broadcast mounting notification */
			if (vconf_set_int(VCONFKEY_SYSMAN_ADDED_USB_STORAGE, added_noti_value) < 0) {
				USB_LOG("Setting vconf value is failed: KEY(%s)", VCONFKEY_SYSMAN_ADDED_USB_STORAGE);
				vconf_set_int(VCONFKEY_SYSMAN_ADDED_USB_STORAGE, -1);
			}

			USB_LOG("Setting vconf value: KEY(%s) DEVICE(%s)", VCONFKEY_SYSMAN_ADDED_USB_STORAGE, buf_dev);
			endmntent(file);

			bundle *b = NULL;
			b = bundle_create();
			bundle_add(b, "_SYSPOPUP_CONTENT_", "otg_add");
			bundle_add(b, "path", buf_mnt_point);
			ret = syspopup_launch("usbotg-syspopup", b);
			if (ret < 0) {
				USB_LOG("popup lauch failed\n");
			}
			bundle_free(b);
			return 0;
		}
	}
	/* Failed to mount storage device */
	USB_LOG("Nothing to be mounted: DEVICE(%s)", buf_dev);
	endmntent(file);
	__USB_FUNC_EXIT__;
	return -1;
}

static int um_usb_storage_removed()
{
	__USB_FUNC_ENTER__;
	int fd = -1;
	char *buf_dev_name = vconf_get_str(VCONFKEY_SYSMAN_REMOVED_STORAGE_UEVENT);
	char buf_mnt_point[BUF_MAX];

	snprintf(buf_mnt_point, BUF_MAX, "%s/%s", MOUNT_POINT, buf_dev_name);

	if (um_unmount_device(buf_mnt_point) == 0) {
		if(umount2(MOUNT_POINT, MNT_DETACH) == 0)
			tmpfs_mounted = 1;
		if (removed_noti_value < INT_MAX) {
			++removed_noti_value;
		} else {
			removed_noti_value = 1;
		}
		if (vconf_set_int(VCONFKEY_SYSMAN_REMOVED_USB_STORAGE, removed_noti_value) < 0) {
			USB_LOG("Setting vconf value is failed: KEY(%s)", VCONFKEY_SYSMAN_REMOVED_USB_STORAGE);
			vconf_set_int(VCONFKEY_SYSMAN_ADDED_USB_STORAGE, -1);
		}
		USB_LOG("Setting vconf value: KEY(%s) DEVICE(%s)", VCONFKEY_SYSMAN_REMOVED_USB_STORAGE, buf_dev_name);
		return 0;
	}
	USB_LOG("Usb storage removed fail");
	__USB_FUNC_EXIT__;
	return -1;
}
