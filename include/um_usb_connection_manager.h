/*
 * Usb Server
 *
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd All Rights Reserved
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

#define SDBD_START "/etc/init.d/sdbd start"
#define SDBD_STOP  "/etc/init.d/sdbd stop"
#define SET_USB0_IP \
			"ifconfig usb0 192.168.129.3 up"
#define UNSET_USB0_IP \
			"ifconfig usb0 down"
#define ADD_DEFAULT_GW \
			"route add -net 192.168.129.0 netmask 255.255.255.0 dev usb0"
#define OPENSSHD_START \
			"/etc/init.d/ssh start"
#define OPENSSHD_STOP \
			"/etc/init.d/ssh stop"

int action_clean(UmMainData *ad, int mode);
int call_cmd(char* cmd);
int connectUsb(UmMainData *ad);
int disconnectUsb(UmMainData *ad);
void change_mode_cb(keynode_t* in_key, void *data);
void change_hotspot_status_cb(keynode_t* in_key, void *data);
static int check_mobile_hotspot_status();
static int run_core_action(UmMainData *ad, int mode);
void select_kies_mode_popup(UmMainData *ad);
int set_USB_mode(UmMainData *ad, int mode);
void change_hotspot_status_cb(keynode_t* in_key, void *data);
void usb_connection_selected_btn(UmMainData *ad, int input);

