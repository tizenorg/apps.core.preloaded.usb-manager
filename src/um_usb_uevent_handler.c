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

#include "um_usb_uevent_handler.h"

#define UEVENT_BUF_MAX (128*1024*1024)
#define UDEV_MON_UDEV "udev"
#define UDEV_ENTRY_NAME_SUBSYSTEM "SUBSYSTEM"
#define UDEV_ENTRY_NAME_ACTION "ACTION"
#define UDEV_ENTRY_NAME_DEVTYPE "DEVTYPE"
#define UDEV_ENTRY_NAME_DEVNAME "DEVNAME"
#define UDEV_ENTRY_NAME_NPARTS "NPARTS"
#define UDEV_ENTRY_NAME_ACCESSORY "ACCESSORY"

#define UDEV_SUBSYSTEM_BLOCK "block"
#define UDEV_SUBSYSTEM_USB_DEVICE "usb_device"
#define UDEV_SUBSYSTEM_USB "usb"
#define UDEV_SUBSYSTEM_MISC "misc"
#define UDEV_ACTION_ADD "add"
#define UDEV_ACTION_REMOVE "remove"
#define UDEV_ACTION_CHANGE "change"
#define UDEV_DEVTYPE_DISK "disk"
#define UDEV_DEVTYPE_PARTITION "partition"
#define UDEV_ACCESSORY_START "START"

static int um_event_control_get_value_by_name(struct udev_list_entry *list_entry,
						char *name, char **value)
{
	__USB_FUNC_ENTER__;
	assert(list_entry);
	assert(name);
	assert(value);

	const char *localValue = NULL;
	struct udev_list_entry *entry_found = NULL;
	entry_found = udev_list_entry_get_by_name(list_entry, name);
	if (!entry_found) {
		USB_LOG("FAIL: udev_list_entry_get_by_name(%s)", name);
		return -1;
	}
	localValue = udev_list_entry_get_value(entry_found);
	if (!localValue) {
		USB_LOG("FAIL: udev_list_entry_get_value(%s)", name);
		return -1;
	}
	USB_LOG("%s: %s", name, localValue);

	*value = strdup(localValue);
	if (*value == NULL) {
		USB_LOG("FAIL: strdup()");
		return -1;
	}

	 __USB_FUNC_EXIT__;
	 return 0;
}

static int um_uevent_control_usb_storage_action(UmMainData *ad,
						char *action,
						char *devname,
						char *fstype)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(action);
	assert(devname);
	assert(fstype);

	if (!strcmp(action, UDEV_ACTION_ADD)) {
		USB_LOG("Mass storage is added");
		um_uevent_mass_storage_added(ad, (char *)devname, fstype);
		__USB_FUNC_EXIT__;
		return 0;

	}

	if (!strcmp(action, UDEV_ACTION_REMOVE)) {
		USB_LOG("Mass storage is removed");
		um_uevent_mass_storage_removed(ad, (char *)devname);
		__USB_FUNC_EXIT__;
		return 0;
	}

	USB_LOG("ERROR: Action (%s) is improper", action);
	__USB_FUNC_EXIT__;
	return -1;
}

static int um_uevent_control_subsystem_block(UmMainData *ad, struct udev_list_entry *list_entry)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(list_entry);

	int ret = -1;
	char *action = NULL; /* add, remove, ... */
	char *devname = NULL; /* /dev/sda, /dev/sda1, ... */
	char *fstype = NULL; /* vfat, ... */
	char *fsversion = NULL; /* fat, fat32, ntfs, ... */

	ret = um_event_control_get_value_by_name(list_entry, "ID_FS_TYPE", &fstype);
	if (ret != 0 || !fstype) {
		USB_LOG("ERROR: This block device cannot be mounted");
		return 0;
	}

	ret = um_event_control_get_value_by_name(list_entry, "ID_FS_VERSION", &fsversion);
	if (ret != 0 || !fsversion) {
		USB_LOG("ERROR: This block device cannot be mounted");
		FREE(fstype);
		return 0;
	}

	/* Getting device name */
	ret = um_event_control_get_value_by_name(list_entry, UDEV_ENTRY_NAME_DEVNAME, &devname);
	if (ret != 0 || !devname) {
		USB_LOG("FAIL: um_event_control_get_value_by_name()");
		goto out_fsversion;
	}
	USB_LOG("The device name is %s", devname);
	if (!strstr(devname, "sd")) {
		USB_LOG("ERROR: devname is improper");
		goto out_devname;
	}

	/* Getting device action */
	ret = um_event_control_get_value_by_name(list_entry, UDEV_ENTRY_NAME_ACTION, &action);
	if (ret != 0 || !action) {
		USB_LOG("FAIL: um_event_control_get_value_by_name()");
		goto out_action;
	}
	USB_LOG("The action is %s", action);

	ret = um_uevent_control_usb_storage_action(ad, action, devname, fstype);
	if (ret < 0) {
		USB_LOG("FAIL: um_uevent_control_usb_storage_action()");
		goto out_action;
	}

	FREE(fstype);
	FREE(fsversion);
	FREE(action);
	FREE(devname);

	__USB_FUNC_EXIT__;
	return 0;

out_action:
	FREE(action);
out_devname:
	FREE(devname);
out_fsversion:
	FREE(fsversion);
	FREE(fstype);
	return -1;
}

static int um_uevent_control_subsystem_usb_device(UmMainData *ad, struct udev_list_entry *list_entry)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(list_entry);

	int ret = -1;
	char *action = NULL; /* add, remove, ... */

	/* Getting device action */
	ret = um_event_control_get_value_by_name(list_entry, UDEV_ENTRY_NAME_ACTION, &action);
	if (ret < 0 || !action) {
		USB_LOG("FAIL: um_event_control_get_value_by_name()");
		FREE(action);
		return -1;
	}
	USB_LOG("The action is %s", action);

	if (!strcmp(action, UDEV_ACTION_ADD)) {
		um_uevent_usb_host_added(ad);
		FREE(action);
		__USB_FUNC_EXIT__;
		return 0;
	}

	if (!strcmp(action, UDEV_ACTION_REMOVE)) {
		um_uevent_usb_host_removed(ad);
		FREE(action);
		__USB_FUNC_EXIT__;
		return 0;
	}

	USB_LOG("ERROR: action (%s) is improper", action);
	FREE(action);
	__USB_FUNC_EXIT__;
	return -1;
}

static int um_uevent_control_subsystem_misc(UmMainData *ad, struct udev_list_entry *list_entry)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(list_entry);

	int ret = -1;
	char *action = NULL; /* add, remove, change ... */
	char *accessory = NULL; /* START, ... */

	/* Getting device action */
	ret = um_event_control_get_value_by_name(list_entry, UDEV_ENTRY_NAME_ACTION, &action);
	if (ret < 0 || !action) {
		USB_LOG("FAIL: um_event_control_get_value_by_name()");
		FREE(action);
		return -1;
	}
	USB_LOG("The action is %s", action);

	ret = um_event_control_get_value_by_name(list_entry, UDEV_ENTRY_NAME_ACCESSORY, &accessory);
	if (ret < 0 || !accessory) {
		USB_LOG("FAIL: um_event_control_get_value_by_name()");
		FREE(action);
		FREE(accessory);
		return -1;
	}
	USB_LOG("The accessory is %s", accessory);

	if (!strcmp(action, UDEV_ACTION_CHANGE) && !strcmp(accessory, UDEV_ACCESSORY_START)) {
		um_uevent_usb_accessory_added(ad);
		FREE(action);
		FREE(accessory);
		__USB_FUNC_EXIT__;
		return 0;
	}

	FREE(action);
	FREE(accessory);
	__USB_FUNC_EXIT__;
	return -1;
}

static void um_uevent_control_subsystem(UmMainData *ad,
					struct udev_list_entry *list_entry,
					char *subsystem)
{
	__USB_FUNC_ENTER__;
	assert(ad);
	assert(list_entry);
	assert(subsystem);

	if (!strcmp(subsystem, UDEV_SUBSYSTEM_BLOCK)) {
		if (um_uevent_control_subsystem_block(ad, list_entry) < 0)
			USB_LOG("FAIL: um_uevent_control_subsystem_block()");
		__USB_FUNC_EXIT__;
		return;
	}

	if (!strcmp(subsystem, UDEV_SUBSYSTEM_USB_DEVICE)) {
		if (um_uevent_control_subsystem_usb_device(ad, list_entry) < 0)
			USB_LOG("FAIL: um_uevent_control_subsystem_usb_device()");
		__USB_FUNC_EXIT__;
		return;
	}

	if (!strcmp(subsystem, UDEV_SUBSYSTEM_MISC)) {
		if (um_uevent_control_subsystem_misc(ad, list_entry) < 0)
			USB_LOG("FAIL: um_uevent_control_subsystem_usb_device()");
		__USB_FUNC_EXIT__;
		return;
	}

	USB_LOG("ERROR: subsystem (%s) is improper", subsystem);
	__USB_FUNC_EXIT__;
}

static Eina_Bool uevent_control_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
	__USB_FUNC_ENTER__;
	if (!data) return ECORE_CALLBACK_RENEW;
	if (!fd_handler) return ECORE_CALLBACK_RENEW;
	UmMainData *ad = (UmMainData *)data;
	int ret = -1;
	int i = 0;
	struct udev_device *dev = NULL;
	struct udev_list_entry *list_entry = NULL;
	struct udev_list_entry *entry_found = NULL;
	char *subsystem = NULL;

	if (!ecore_main_fd_handler_active_get(ad->udevFdHandler, ECORE_FD_READ)) {
		USB_LOG("FAIL: ecore_main_fd_handler_active_get()");
		return ECORE_CALLBACK_RENEW;
	}
	dev = udev_monitor_receive_device(ad->udevMon);
	if (!dev) {
		USB_LOG("FAIL: udev_monitor_receive_device()");
		return ECORE_CALLBACK_RENEW;
	}

	/* Getting the First element of the device entry list */
	list_entry = udev_device_get_properties_list_entry(dev);
	if (!list_entry) {
		USB_LOG("FAIL: udev_device_get_properties_list_entry()");
		udev_device_unref(dev);
		return ECORE_CALLBACK_RENEW;
	}

	ret = um_event_control_get_value_by_name(list_entry, UDEV_ENTRY_NAME_SUBSYSTEM, &subsystem);
	if (ret < 0 || !subsystem) {
		USB_LOG("FAIL: um_event_control_get_value_by_name()");
		udev_device_unref(dev);
		return ECORE_CALLBACK_RENEW;
	}

	udev_list_entry_foreach(entry_found, list_entry) {
		USB_LOG("::::::::::::::::::::::::::::::::");
		USB_LOG("::: Number: %d", i);
		USB_LOG("::: Name  : %s", udev_list_entry_get_name(entry_found));
		USB_LOG("::: value : %s", udev_list_entry_get_value(entry_found));
	}
	USB_LOG(":::::::::::::::::::::::::::::");

	um_uevent_control_subsystem(ad, list_entry, subsystem);

	udev_device_unref(dev);
	FREE(subsystem);
	__USB_FUNC_EXIT__;
	return ECORE_CALLBACK_RENEW;
}

void um_uevent_control_stop(UmMainData *ad)
{
	__USB_FUNC_ENTER__;
	if (!ad) return ;

	if (ad->udevFdHandler) {
		ecore_main_fd_handler_del(ad->udevFdHandler);
		ad->udevFdHandler = NULL;
	}

	if (ad->udevFd >= 0) {
		close(ad->udevFd);
		ad->udevFd = -1;
	}

	if (ad->udevMon) {
		udev_monitor_unref(ad->udevMon);
		ad->udevMon = NULL;
	}

	if (ad->udev) {
		udev_unref(ad->udev);
		ad->udev = NULL;
	}

	__USB_FUNC_EXIT__;
}

int um_uevent_control_start(UmMainData *ad, int mode)
{
	__USB_FUNC_ENTER__;
	if (!ad) return -1;
	int ret = -1;
	ad->udevFd = -1;

	ad->udev = udev_new();
	if (!(ad->udev)) {
		USB_LOG("FAIL: udev_new()");
		return -1;
	}

	ad->udevMon = udev_monitor_new_from_netlink(ad->udev, UDEV_MON_UDEV);
	if (ad->udevMon == NULL) {
		USB_LOG("FAIL: udev_monitor_new_from_netlink()");
		um_uevent_control_stop(ad);
		return -1;
	}

	/* The buffer size should be 128 * 1024 * 1024
	 * according to the buffer size of udev daemon */
	ret = udev_monitor_set_receive_buffer_size(ad->udevMon, UEVENT_BUF_MAX);
	if (ret < 0) {
		USB_LOG("FAIL: udev_monitor_set_receive_buffer_size()");
		um_uevent_control_stop(ad);
		return -1;
	}

	switch (mode) {
	case USB_DEVICE_HOST:
		if (udev_monitor_filter_add_match_subsystem_devtype(ad->udevMon,
					UDEV_SUBSYSTEM_BLOCK, NULL) < 0
			|| udev_monitor_filter_add_match_subsystem_devtype(ad->udevMon,
					UDEV_SUBSYSTEM_USB, NULL) < 0
			|| udev_monitor_filter_add_match_subsystem_devtype(ad->udevMon,
					UDEV_SUBSYSTEM_USB_DEVICE, NULL) < 0) {
			USB_LOG("FAIL: udev_monitor_filter_add_match_subsystem_devtype()");
			um_uevent_control_stop(ad);
			return -1;
		}
		break;

	case USB_DEVICE_CLIENT:
		if (udev_monitor_filter_add_match_subsystem_devtype(ad->udevMon,
					UDEV_SUBSYSTEM_MISC, NULL) < 0) {
			USB_LOG("FAIL: udev_monitor_filter_add_match_subsystem_devtype()");
			um_uevent_control_stop(ad);
			return -1;
		}

		break;

	default:
		USB_LOG("ERROR: mode (%d) is improper", mode);
		um_uevent_control_stop(ad);
		return -1;
	}

	ad->udevFd = udev_monitor_get_fd(ad->udevMon);
	if (ad->udevFd < 0) {
		USB_LOG("FAIL: udev_monitor_get_fd()");
		um_uevent_control_stop(ad);
		return -1;
	}

	ad->udevFdHandler = ecore_main_fd_handler_add(ad->udevFd, ECORE_FD_READ,
						uevent_control_cb, ad, NULL, NULL);
	if (ad->udevFdHandler == NULL) {
		USB_LOG("FAIL: ecore_main_fd_handler_add()");
		um_uevent_control_stop(ad);
		return -1;
	}

	ret = udev_monitor_enable_receiving(ad->udevMon);
	if (ret < 0) {
		USB_LOG("FAIL: udev_monitor_enable_receiving()");
		um_uevent_control_stop(ad);
		return -1;
	}

	__USB_FUNC_EXIT__;
	return 0;
}

