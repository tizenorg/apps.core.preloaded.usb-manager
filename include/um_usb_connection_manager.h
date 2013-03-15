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

#ifndef __UM_USB_CONNECTION_MANAGER_H__
#define __UM_USB_CONNECTION_MANAGER_H__

#include "um_customize.h"

int connectUsbClient(UmMainData *ad);
void disconnectUsbClient(UmMainData *ad);
void action_clean(UmMainData *ad, int mode);
void debug_mode_cb(keynode_t* in_key, void *data);
void change_mode_cb(keynode_t* in_key, void *data);
void change_prev_mode_cb(keynode_t* in_key, void *data);
void change_hotspot_status_cb(keynode_t* in_key, void *data);
void usb_connection_selected_btn(UmMainData *ad, int input);
int launch_acc_app(char *appId);
Eina_Bool hasAccPermission(UmMainData *ad, char *appId);

#endif /* __UM_USB_CONNECTION_MANAGER_H__ */

