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

#ifndef __UM_USB_NOTIFICATION_H__
#define __UM_USB_NOTIFICATION_H__

#include "um_common.h"

int um_append_host_notification(UmMainData *ad,
				int notiType,
				void *device,
				char *_title,
				char *_content,
				char *_iconPath);

int um_append_client_notification(UmMainData *ad,
				int notiType,
				void *device,
				char *title,
				char *content,
				char *iconPath);

int um_remove_host_notification(UmMainData *ad,
				int notiType,
				void *device);

int um_remove_client_notification(UmMainData *ad,
				int notiType,
				void *device);

int um_remove_all_client_notification(UmMainData *ad);

int um_remove_all_host_notification(UmMainData *ad);

#endif /* __UM_USB_NOTIFICATION_H__ */

