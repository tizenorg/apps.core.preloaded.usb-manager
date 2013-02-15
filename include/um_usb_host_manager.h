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
#include <sys/mount.h>
#include <mntent.h>
#include <string.h>
#include "um_customize.h"

#define MTAB_FILE       "/etc/mtab"
#define MOUNT_POINT     "/opt/storage/usb"



int umGetDeviceInfo(UmMainData *ad);
void destroy_device(gpointer data);
int umReleaseAllDevice(UmMainData *ad);
void usb_host_added_cb(UmMainData *ad);
void usb_host_removed_cb(UmMainData *ad);
int find_host_fd(UmMainData *ad, char *appId);
int grantHostPermission(UmMainData *ad, char *appId, int vendor, int product);
int launch_host_app(char *appId);
Eina_Bool hasHostPermission(UmMainData *ad, char *appId, int vendor, int product);
int show_all_usb_devices(GList *devList, int option);
void free_func(gpointer data);
static int um_usb_storage_added();
static int um_usb_storage_removed();
int disconnectUsbHost(UmMainData *ad);

void add_host_noti_cb(void *data);
void remove_host_noti_cb(void *data);
void add_mass_storage_cb(keynode_t *in_key, void *data);
void remove_mass_storage_cb(keynode_t *in_key, void *data);
