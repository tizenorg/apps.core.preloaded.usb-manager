/*
 * Usb Server
 *
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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
#include <vconf.h>
#include <appsvc.h>

#define USB_ACCESSORY_GET_MANUFACTURER	_IOW('M', 1, char[256])
#define USB_ACCESSORY_GET_MODEL			_IOW('M', 2, char[256])
#define USB_ACCESSORY_GET_DESCRIPTION	_IOW('M', 3, char[256])
#define USB_ACCESSORY_GET_VERSION		_IOW('M', 4, char[256])
#define USB_ACCESSORY_GET_URI			_IOW('M', 5, char[256])
#define USB_ACCESSORY_GET_SERIAL		_IOW('M', 6, char[256])

int getAccessoryInfo(UsbAccessory *usbAcc);
int connectAccessory(UmMainData *ad);
int disconnectAccessory(UmMainData *ad);
void getCurrentAccessory();
void umAccInfoInit(UmMainData *ad);
