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

#ifndef __UM_USB_HOST_MANAGER_H__
#define __UM_USB_HOST_MANAGER_H__

#include <vconf.h>
#include <sys/mount.h>
#include <mntent.h>
#include <string.h>
#include "um_customize.h"
#include "um_usb_notification.h"

#define MTAB_FILE       "/etc/mtab"
#define MOUNT_POINT     tzplatform_mkpath(TZ_SYS_STORAGE, "usb")

int um_get_device_info(UmMainData *ad);
void destroy_device(gpointer data);
void usb_host_added_cb(UmMainData *ad);
void usb_host_removed_cb(UmMainData *ad);
int find_host_fd(UmMainData *ad, char *appId);
int grant_host_permission(UmMainData *ad, char *appId, int vendor, int product);
int launch_host_app(char *appId);
Eina_Bool has_host_permission(UmMainData *ad, char *appId, int vendor, int product);
void disconnect_usb_host(UmMainData *ad);
void show_all_usb_devices(GList *devList);
Eina_Bool is_host_connected(UmMainData *ad, int vendor, int product);
bool is_mass_storage_mounted(UmMainData *ad, char *devname);

void um_uevent_usb_host_added(UmMainData *ad);
void um_uevent_usb_host_removed(UmMainData *ad);
void um_uevent_mass_storage_added(UmMainData *ad, char *devname, char *fstype);
void um_uevent_mass_storage_removed(UmMainData *ad, char *devname);

#endif /* __UM_USB_HOST_MANAGER_H__ */
