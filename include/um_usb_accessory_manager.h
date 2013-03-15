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

#ifndef __UM_USB_ACCESSORY_MANAGER_H__
#define __UM_USB_ACCESSORY_MANAGER_H__

#include "um_customize.h"
#include <vconf.h>
#include <appsvc.h>

#define USB_ACCESSORY_GET_MANUFACTURER	_IOW('M', 1, char[256])
#define USB_ACCESSORY_GET_MODEL			_IOW('M', 2, char[256])
#define USB_ACCESSORY_GET_DESCRIPTION	_IOW('M', 3, char[256])
#define USB_ACCESSORY_GET_VERSION		_IOW('M', 4, char[256])
#define USB_ACCESSORY_GET_URI			_IOW('M', 5, char[256])
#define USB_ACCESSORY_GET_SERIAL		_IOW('M', 6, char[256])

void disconnect_accessory(UmMainData *ad);
void accessory_info_init(UmMainData *ad);
void get_current_accessory();
int grant_accessory_permission(UmMainData *ad, char *appId);
bool has_accessory_permission(UmMainData *ad, char *appId);
void um_uevent_usb_accessory_added(UmMainData *ad);

#endif /* __UM_USB_ACCESSORY_MANAGER_H__ */
