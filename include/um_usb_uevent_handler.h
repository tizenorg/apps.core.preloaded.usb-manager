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

#ifndef __UM_USB_UEVENT_HANDLER_H__
#define __UM_USB_UEVENT_HANDLER_H__

#include "um_usb_host_manager.h"
#include "um_usb_accessory_manager.h"

int um_uevent_control_start(UmMainData *ad, int mode);
void um_uevent_control_stop(UmMainData *ad);

#endif /* __UM_UEVENT_HANDLER_H__ */
