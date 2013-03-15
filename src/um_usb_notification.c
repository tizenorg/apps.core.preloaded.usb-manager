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

#include "um_usb_notification.h"

static void release_usb_notification(gpointer data)
{
	__USB_FUNC_ENTER__ ;
	assert(data);
	UsbNotification *usbNoti = (UsbNotification *)data;
	FREE(usbNoti->title);
	FREE(usbNoti->content);
	FREE(usbNoti->iconPath);
	FREE(usbNoti->devName);
	FREE(usbNoti->devIf);
	FREE(usbNoti->accDev);

	__USB_FUNC_EXIT__ ;
}

static int alloc_usb_notification(UsbNotification **usbNoti,
				int privId,
				char *title,
				char *content,
				char *iconPath,
				int applist,
				char *devName,
				UsbInterface *devIf,
				UsbAccessory *accDev)
{
	__USB_FUNC_ENTER__ ;
	assert(usbNoti);
	assert(title);
	assert(content);
	assert(iconPath);

	if (*usbNoti) release_usb_notification(*usbNoti);
	*usbNoti = (UsbNotification *)malloc(sizeof(UsbNotification));
	if (!(*usbNoti)) return -1;
	(*usbNoti)->privId = privId;
	(*usbNoti)->title = strdup(title);
	(*usbNoti)->content = strdup(content);
	(*usbNoti)->iconPath = strdup(iconPath);
	(*usbNoti)->applist = applist;

	if (devName) (*usbNoti)->devName = strdup(devName);
	else (*usbNoti)->devName = NULL;

	(*usbNoti)->devIf = devIf;
	(*usbNoti)->accDev = accDev;

	__USB_FUNC_EXIT__ ;
	return 0;
}

static int cmpTwoDev(int notiType, void *device, UsbNotification *usbNoti)
{
	__USB_FUNC_ENTER__ ;
	assert(device);
	assert(usbNoti);
	UsbInterface *devIf = NULL;
	UsbInterface *notiDevIf = NULL;
	UsbAccessory *accDev = NULL;
	UsbAccessory *notiAccDev = NULL;

	switch(notiType) {
	case USB_NOTI_HOST_ADDED:
	case USB_NOTI_HOST_REMOVED:
		devIf = (UsbInterface *)device;
		notiDevIf = usbNoti->devIf;
		if (devIf->ifIdVendor == notiDevIf->ifIdVendor
			&& devIf->ifIdProduct == notiDevIf->ifIdProduct
			&& devIf->ifBus == notiDevIf->ifBus
			&& devIf->ifAddress == notiDevIf->ifAddress) {
			USB_LOG("Two devices are same");
			__USB_FUNC_EXIT__ ;
			return 0;
		}
		break;
	case USB_NOTI_ACC_ADDED:
	case USB_NOTI_ACC_REMOVED:
		accDev = (UsbAccessory *)device;
		notiAccDev = usbNoti->accDev;
		if (!strncmp(accDev->manufacturer, notiAccDev->manufacturer,
					strlen(accDev->manufacturer))
			&& !strncmp(accDev->model, notiAccDev->model, strlen(accDev->model))
			&& !strncmp(accDev->version, notiAccDev->version, strlen(accDev->version))
			&& !strncmp(accDev->serial, notiAccDev->serial, strlen(accDev->serial))) {
			USB_LOG("Two accessories are same");
			__USB_FUNC_EXIT__ ;
			return 0;
		} else {
			USB_LOG("device:  %s, %s, %s, %s", accDev->manufacturer, accDev->model, accDev->version, accDev->serial);
			USB_LOG("notiDev: %s, %s, %s, %s", notiAccDev->manufacturer, notiAccDev->model, notiAccDev->version, notiAccDev->serial);
		}
		break;
	default:
		break;
	}
	__USB_FUNC_EXIT__ ;
	return -1;
}

static int show_noti_list(UmMainData *ad, GList *notiList)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	int cnt = 0;
	UsbNotification *usbNoti = NULL;
	GList *l = NULL;
	USB_LOG("*****************************");
	USB_LOG("** Elements of list **");
	if (!notiList) {
		USB_LOG("** No element in the List");
		USB_LOG("****************************");
		return 0;
	}

	cnt = g_list_length(notiList);
	USB_LOG("** g_list_length(notiList): %d", cnt);

	for(l = notiList ; l ; l = g_list_next(l)) {
		usbNoti = (UsbNotification *)(l->data);
		if (!usbNoti) continue;
		USB_LOG("** privId: %d", usbNoti->privId);
		USB_LOG("** title: %s", usbNoti->title);
		USB_LOG("** content: %s", usbNoti->content);
		USB_LOG("** iconPath %s", usbNoti->iconPath);
	}
	USB_LOG("****************************");
	__USB_FUNC_EXIT__ ;
	return 0;
}

static bool cmp_dev_noti(int notiType, UsbNotification *usbNoti, void *device)
{
	__USB_FUNC_ENTER__ ;
	assert(usbNoti);
	assert(device);

	char *devName = NULL;
	UsbInterface *devIf = NULL;
	UsbInterface *hDevIf = NULL;
	UsbAccessory *accDev = NULL;
	UsbAccessory *aDev = NULL;

	switch (notiType) {
	case USB_NOTI_MASS_STORAGE_ADDED:
	case USB_NOTI_MASS_STORAGE_REMOVED:
		USB_LOG("Mass storage");
		devName = (char *)device;
		USB_LOG("%s, %s", usbNoti->devName, devName);
		if (!strncmp(usbNoti->devName, devName, strlen(devName))) {
			return true;
		} else {
			return false;
		}
	case USB_NOTI_HOST_ADDED:
	case USB_NOTI_HOST_REMOVED:
		USB_LOG("USB host");
		devIf = (UsbInterface *)device;
		hDevIf = usbNoti->devIf;

		if (hDevIf->ifIdVendor == devIf->ifIdVendor
			&& hDevIf->ifIdProduct == devIf->ifIdProduct
			&& hDevIf->ifBus == devIf->ifBus
			&& hDevIf->ifAddress == devIf->ifAddress) {
			USB_LOG("return true");
			return true;
		} else {
			USB_LOG("return false");
			return false;
		}
	case USB_NOTI_ACC_ADDED:
	case USB_NOTI_ACC_REMOVED:
		USB_LOG("USB accessory");
		accDev = (UsbAccessory *)device;
		aDev = usbNoti->accDev;
		if ( !strncmp(aDev->manufacturer, accDev->manufacturer, strlen(accDev->manufacturer))
			&& !strncmp(aDev->model, accDev->model, strlen(accDev->model))
			&& !strncmp(aDev->version, accDev->version, strlen(accDev->version))
			&& !strncmp(aDev->serial, accDev->serial, strlen(accDev->serial))) {
			return true;
		} else {
			return false;
		}
	default:
		USB_LOG("notiType is invalid");
		break;
	}
	return false;
}

static int um_set_notification_info(notification_h noti,
					char *title,
					char *content,
					char *iconPath,
					int applist)
{
	__USB_FUNC_ENTER__ ;
	assert(noti);
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	if (title) {
		/* Set Title of notification */
		noti_err = notification_set_text(noti,
					NOTIFICATION_TEXT_TYPE_TITLE,
					title,
					NULL,
					NOTIFICATION_VARIABLE_TYPE_NONE);
		if(noti_err != NOTIFICATION_ERROR_NONE) {
			USB_LOG("FAIL: notification_set_text(): %d", noti_err);
			return -1;
		}
	}

	if (content) {
		/* Set Subtitle of notification */
		noti_err = notification_set_text(noti,
					NOTIFICATION_TEXT_TYPE_CONTENT,
					content,
					NULL,
					NOTIFICATION_VARIABLE_TYPE_NONE);
		if(noti_err != NOTIFICATION_ERROR_NONE) {
			USB_LOG("FAIL: notification_set_text(): %d", noti_err);
			return -1;
		}
	}

	if (iconPath) {
		/* Set Icon image of notification */
		noti_err = notification_set_image(noti,
					NOTIFICATION_IMAGE_TYPE_ICON,
					USB_ICON_PATH);
		if(noti_err != NOTIFICATION_ERROR_NONE) {
			USB_LOG("FAIL: notification_set_image(): %d", noti_err);
			return -1;
		}
	}

	if (applist > 0) {
		/* Set apps which should receive this noftification */
		noti_err = notification_set_display_applist(noti, applist);
		if(noti_err != NOTIFICATION_ERROR_NONE) {
			USB_LOG("FAIL: notification_set_display_applist(): %d", noti_err);
			return -1;
		}
	}

	__USB_FUNC_EXIT__;
	return 0;
}

static int um_add_notification(UmMainData *ad,
			int notiType,
			char *title,
			char *content,
			char *iconPath,
			int applist)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(title);
	assert(content);
	assert(iconPath);

	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	int privId = -1;
	int ret = -1;

	/* Create new notification */
	noti = notification_new(NOTIFICATION_TYPE_ONGOING,
				NOTIFICATION_GROUP_ID_NONE,
				NOTIFICATION_PRIV_ID_NONE);
	if(!noti) {
		USB_LOG("FAIL: notification_new()");
		return -1;
	}

	/* Set Package name of notification */
	noti_err = notification_set_pkgname(noti, PACKAGE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		USB_LOG("FAIL: notification_set_text(): %d", noti_err);
		noti_err = notification_free(noti);
		if (noti_err != NOTIFICATION_ERROR_NONE)
			USB_LOG("FAIL: notification_free(noti)");
		return -1;
	}

	/* Set nofification info (title, content, icon, applist) */
	ret = um_set_notification_info(noti, title, content, iconPath, applist);
	if (ret < 0) {
		USB_LOG("FAIL: um_set_notification_info()");
		noti_err = notification_free(noti);
		if (noti_err != NOTIFICATION_ERROR_NONE)
			USB_LOG("FAIL: notification_free(noti)");
		return -1;
	}

	/*************************************************/
	/* TODO: When clicking the notification of Mass storage,
	 * Unmounting the storage should be proceeded.
	 * This feature will be added later */
	switch (notiType) {
	case USB_NOTI_MASS_STORAGE_ADDED:
		break;
	default:
		break;
	}
	/*************************************************/

	/* Insert notification and get private id */
	noti_err = notification_insert(noti, &privId);
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		USB_LOG("FAIL: notification_insert(): %d", noti_err);
		noti_err = notification_free(noti);
		if (noti_err != NOTIFICATION_ERROR_NONE)
			USB_LOG("FAIL: notification_free(noti)");
		return -1;
	}
	USB_LOG("Private id of the notification: %d", privId);

	noti_err = notification_free(noti);
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		USB_LOG("FAIL: notification_insert(): %d", noti_err);
	}

	__USB_FUNC_EXIT__;
	return privId;
}

static int um_update_add_notification(UmMainData *ad,
			int privId,
			int notiType,
			char *title,
			char *content,
			char *iconPath,
			int applist)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(title);
	assert(content);
	assert(iconPath);

	int ret = -1;
	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	if (privId < 0) {
		USB_LOG("This is first notification of %d", notiType);
		ret = um_add_notification(ad, notiType, title, content, iconPath, applist);
		if (ret < 0) USB_LOG("FAIL: um_add_notification()");
		return ret;
	} else { /* privId >= 0 */

		/* Load notification with privId */
		noti = notification_load(PACKAGE, privId);
		if (!noti) {
			USB_LOG("FAIL: notification_load()");
			return -1;
		}

		/* Set nofification info (title, content, icon, applist) */
		ret = um_set_notification_info(noti, title, content, iconPath, applist);
		if (ret < 0) {
			USB_LOG("FAIL: um_set_notification_info()");
			noti_err = notification_free(noti);
			if (noti_err != NOTIFICATION_ERROR_NONE)
				USB_LOG("FAIL: notification_free(noti)");
			return -1;
		}

		/* Update notification */
		noti_err = notification_update(noti);
		if(noti_err != NOTIFICATION_ERROR_NONE) {
			USB_LOG("FAIL: notification_update(): %d", noti_err);
			noti_err = notification_free(noti);
			if (noti_err != NOTIFICATION_ERROR_NONE)
				USB_LOG("FAIL: notification_free(noti)");
			return -1;
		}

		noti_err = notification_free(noti);
		if(noti_err != NOTIFICATION_ERROR_NONE) {
			USB_LOG("FAIL: notification_insert(): %d", noti_err);
		}
	}

	__USB_FUNC_EXIT__;
	return privId;
}

static int um_append_separate_notification_mass_storage(UmMainData *ad,
						int notiType,
						char *device,
						char *title,
						char *content,
						char *iconPath)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(device);
	assert(title);

	int privId = -1;
	int ret = -1;
	UsbNotification *usbNoti = NULL;

	if (notiType != USB_NOTI_MASS_STORAGE_ADDED) {
		USB_LOG("ERROR: notiType is improper");
		return -1;
	}

	ret = show_noti_list(ad, ad->notiMSList);
	if (ret < 0) USB_LOG("FAIL: show_noti_list()");

	privId = um_add_notification(ad, notiType, title, content, iconPath,
				NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY);
	um_retvm_if(privId < 0, -1, "FAIL: um_add_notification()");

	if (alloc_usb_notification(&usbNoti, privId, title, content, iconPath,
				NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY,
				device, NULL, NULL) < 0) {
		USB_LOG("FAIL: alloc_usb_notification(&usbNoti)");
		return -1;
	}
	ad->notiMSList = g_list_append(ad->notiMSList, usbNoti);

	ret = show_noti_list(ad, ad->notiMSList);
	if (ret < 0) USB_LOG("FAIL: show_noti_list()");

	__USB_FUNC_EXIT__;
	return privId;
}

static int um_append_separate_notification(UmMainData *ad,
					int notiType,
					void *device,
					char *title,
					char *content,
					char *iconPath)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(device);
	assert(title);

	int privId = -1;

	switch (notiType) {
	case USB_NOTI_MASS_STORAGE_ADDED:
		privId = um_append_separate_notification_mass_storage(ad, notiType,
						device, title, content, iconPath);
		if (privId < 0) {
			USB_LOG("FAIL: um_append_separate_notification_mass_storage()");
			return -1;
		}
		break;
	default:
		USB_LOG("This noti type does not support multi notification");
		return -1;
	}

	__USB_FUNC_EXIT__;
	return privId;
}

static int um_append_cumulative_notification_host_acc(UmMainData *ad,
						int notiType,
						void *device,
						char *_title,
						char *_content,
						char *_iconPath,
						UsbNotification **usbNoti)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(device);
	assert(_title);
	assert(usbNoti);

	int privId = -1;
	UsbInterface *devIf = NULL;
	UsbAccessory *accDev = NULL;
	GList *notiList = NULL;
	char titleBuf[BUF_MAX];
	char *title = _title;
	char *content = _content;
	char *iconPath = _iconPath;
	int cnt = -1;

	switch (notiType) {
	case USB_NOTI_HOST_ADDED:
		notiList = ad->notiHostList;
		devIf = (UsbInterface *)device;
		break;
	case USB_NOTI_ACC_ADDED:
		notiList = ad->notiAccList;
		accDev = (UsbAccessory *)device;
		break;
	default:
		USB_LOG("This noti type does not support multi notification");
		return -1;
	}

	if (notiList) {
		privId = ((UsbNotification *)g_list_nth_data(notiList, 0))->privId;
		cnt = (int)g_list_length(notiList);
		USB_LOG("Number of elements of notiList: %d", cnt);
		if (cnt >= 1) {
			snprintf(titleBuf, sizeof(titleBuf), "%d USB devices connected", cnt+1);
			title = titleBuf;
			content = "Tap to show devices";
		} else {
			privId = -1;
		}
	} else {
			privId = -1;
	}
	USB_LOG("privId: %d", privId);

	privId = um_update_add_notification(ad, privId, notiType, title, content, iconPath,
					NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY);
	um_retvm_if(privId < 0, -1, "FAIL: um_update_add_notification()");

	if (alloc_usb_notification(usbNoti, privId, _title, _content, _iconPath,
				NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY,
				NULL, devIf, accDev) < 0) {
		USB_LOG("FAIL: alloc_usb_notification(&usbNoti)");
		return -1;
	}

	__USB_FUNC_EXIT__;
	return privId;
}

static int um_append_cumulative_notification(UmMainData *ad,
						int notiType,
						void *device,
						char *title,
						char *content,
						char *iconPath)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(device);
	assert(title);

	int privId = -1;
	UsbNotification *usbNoti = NULL;

	privId = um_append_cumulative_notification_host_acc(ad, notiType, device,
						title, content, iconPath, &usbNoti);
	if (privId < 0 || !usbNoti) {
		USB_LOG("FAIL: um_append_cumulative_notification_host()");
		return -1;
	}

	switch (notiType) {
	case USB_NOTI_HOST_ADDED:
		ad->notiHostList = g_list_append(ad->notiHostList, usbNoti);
		break;
	case USB_NOTI_ACC_ADDED:
		ad->notiAccList = g_list_append(ad->notiAccList, usbNoti);
		break;
	default:
		USB_LOG("This noti type does not support multi notification");
		return -1;
	}

	__USB_FUNC_EXIT__;
	return privId;
}

int um_append_host_notification(UmMainData *ad,
				int notiType,
				void *device,
				char *_title,
				char *_content,
				char *_iconPath)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(device);
	assert(_title);

	int privId = -1;
	UsbInterface *devIf = NULL;
	char *devName = NULL;
	char *title = _title;
	char *content = _content;
	char *iconPath = _iconPath;

	switch (notiType) {
	case USB_NOTI_MASS_STORAGE_ADDED:
		devName = (char *)device;
		privId = um_append_separate_notification(ad,
					notiType,
					devName,
					title,
					content,
					iconPath);
		um_retvm_if(privId < 0, -1, "FAIL: um_add_notification()");
		break;
	case USB_NOTI_HOST_ADDED:
		devIf = (UsbInterface *)device;
		privId = um_append_cumulative_notification(ad,
					notiType,
					devIf,
					title,
					content,
					iconPath);
		um_retvm_if(privId < 0, -1, "FAIL: um_append_multi_notification()");
		break;
	default:
		break;
	}
	USB_LOG("privId: %d", privId);

	__USB_FUNC_EXIT__;
	return 0;
}

int um_append_client_notification(UmMainData *ad,
				int notiType,
				void *device,
				char *title,
				char *content,
				char *iconPath)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(title);

	int privId = -1;
	if (notiType != USB_NOTI_ACC_ADDED) {
		USB_LOG("ERROR: This function is just for usb accessory");
		return -1;
	}

	privId = um_append_cumulative_notification(ad,
					notiType,
					device,
					title,
					content,
					iconPath);
	um_retvm_if(privId < 0, -1, "FAIL: um_append_multi_notification()");

	__USB_FUNC_EXIT__;
	return 0;
}

static int um_remove_notification(UmMainData *ad,
			int notiType,
			int privId)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);

	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	/* Delete notification */
	noti_err = notification_delete_by_priv_id(PACKAGE,
						NOTIFICATION_TYPE_ONGOING,
						privId);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		USB_LOG("FAIL: notification_delete_by_priv_id(): %d", noti_err);
		return -1;
	}

	__USB_FUNC_EXIT__;
	return privId;
}

static int um_update_remove_notification(UmMainData *ad,
					int notiType,
					int privId,
					void *device)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(device);

	int ret = -1;
	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	UsbNotification *usbNoti = NULL;
	GList *notiList = NULL;
	int cnt = -1;
	int i = -1;
	char title[BUF_MAX];
	char *content;
	char *iconPath;
	UsbInterface *devIf = NULL;
	UsbAccessory *accDev = NULL;
	int applist = -1;

	switch (notiType) {
	case USB_NOTI_HOST_REMOVED:
		notiList = ad->notiHostList;
		devIf = (UsbInterface *)device;
		break;
	case USB_NOTI_ACC_REMOVED:
		notiList = ad->notiAccList;
		accDev = (UsbAccessory *)device;
		break;
	default:
		break;
	}
	um_retvm_if(!notiList, -1, "ERROR: notiList == NULL");

	cnt = g_list_length(notiList);
	USB_LOG("g_list_length(notiList): %d", cnt);
	if (cnt <= 0) {
		USB_LOG("FAIL: g_list_length(notiList)");
		return -1;
	} else if (cnt == 1) {
		privId = um_remove_notification(ad, notiType, privId);
		um_retvm_if(privId < 0, -1, "FAIL: um_remove_notification()");
	} else if (cnt >= 2) {
		/* Load notification with privId */
		noti = notification_load(PACKAGE, privId);
		if (!noti) {
			USB_LOG("FAIL: notification_load()");
			return -1;
		}

		/* Set title, content, iconPath, applist of Notification */
		if (cnt == 2) {
			for (i = 0 ; i < cnt ; i++) {
				usbNoti = (UsbNotification *)g_list_nth_data(notiList, i);
				if (0 != cmpTwoDev(notiType, device, usbNoti)) {
					break;
				}
			}
			snprintf(title, BUF_MAX, "%s", usbNoti->title);
			content = usbNoti->content;
			iconPath = usbNoti->iconPath;
			applist = usbNoti->applist;

		} else { /* cnt >= 3 */
			snprintf(title, BUF_MAX, "%d USB devices connected", cnt - 1);
			USB_LOG("title: %s", title);
			content = "Tap to show devices";
			iconPath = USB_ICON_PATH;
			applist = NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY;
		}

		/* Set nofification info (title, content, icon, applist) */
		ret = um_set_notification_info(noti, title, content, iconPath, applist);
		if (ret < 0) {
			USB_LOG("FAIL: um_set_notification_info()");
			noti_err = notification_free(noti);
			if (noti_err != NOTIFICATION_ERROR_NONE)
				USB_LOG("FAIL: notification_free(noti)");
			return -1;
		}

		/* Update notification */
		noti_err = notification_update(noti);
		if(noti_err != NOTIFICATION_ERROR_NONE) {
			USB_LOG("FAIL: notification_update(): %d", noti_err);
			noti_err = notification_free(noti);
			if (noti_err != NOTIFICATION_ERROR_NONE)
				USB_LOG("FAIL: notification_free(noti)");
			return -1;
		}

		noti_err = notification_free(noti);
		if(noti_err != NOTIFICATION_ERROR_NONE) {
			USB_LOG("FAIL: notification_insert(): %d", noti_err);
		}
	}

	__USB_FUNC_EXIT__;
	return privId;
}

static int um_remove_separate_notification_mass_storage(UmMainData *ad,
							int notiType,
							void *device)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(device);

	int privId = -1;
	int ret = -1;
	UsbNotification *usbNoti = NULL;
	GList *l = NULL;
	char *devName = (char *)device;

	if (notiType != USB_NOTI_MASS_STORAGE_REMOVED) {
		USB_LOG("ERROR: notiType (%d) is improper", notiType);
		return -1;
	}

	ret = show_noti_list(ad, ad->notiMSList);
	if (ret < 0) USB_LOG("FAIL: show_noti_list()");

	for (l = ad->notiMSList ; l ; l = g_list_next(l)) {
		usbNoti = (UsbNotification *)(l->data);
		if (!usbNoti) {
			USB_LOG("FAIL: usbNoti == NULL");
			return -1;
		}
		USB_LOG("privId: %d", usbNoti->privId);
		if (true == cmp_dev_noti(notiType, usbNoti, devName)) {
			privId = um_remove_notification(ad, notiType, usbNoti->privId);
			um_retvm_if(privId < 0, -1, "FAIL: um_remove_notification()");
			ad->notiMSList = g_list_delete_link(ad->notiMSList, l);
			break;
		}
	}

	ret = show_noti_list(ad, ad->notiMSList);
	if (ret < 0) USB_LOG("FAIL: show_noti_list()");

	__USB_FUNC_EXIT__;
	return privId;
}

static int um_remove_separate_notification(UmMainData *ad, int notiType, void *device)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(device);

	int privId = -1;

	switch (notiType) {
	case USB_NOTI_MASS_STORAGE_REMOVED:
		privId = um_remove_separate_notification_mass_storage(ad, notiType, device);
		if (privId < 0) {
			USB_LOG("FAIL: um_remove_separate_notification_mass_storage()");
			return -1;
		}
		break;
	default:
		USB_LOG("This noti type does not support multi notification");
		return -1;
	}

	__USB_FUNC_EXIT__;
	return privId;
}

static int um_remove_cumulative_notification_host_acc(UmMainData *ad,
						int notiType,
						void *device,
						GList **rmList)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(device);
	assert(rmList);

	int privId = -1;
	UsbInterface *devIf = NULL;
	UsbAccessory *accDev = NULL;
	UsbNotification *usbNoti = NULL;
	GList *notiList = NULL;
	GList *l = NULL;

	switch (notiType) {
	case USB_NOTI_HOST_REMOVED:
		notiList = ad->notiHostList;
		devIf = (UsbInterface *)device;
		if (devIf->ifClass == USB_HOST_HUB) return 0;
		break;
	case USB_NOTI_ACC_REMOVED:
		notiList = ad->notiAccList;
		accDev = (UsbAccessory *)device;
		break;
	default:
		USB_LOG("This noti type does not support multi notification");
		return -1;
	}

	for (l = notiList ; l ; l = g_list_next(l)) {
		usbNoti = (UsbNotification *)(l->data);
		if (true == cmp_dev_noti(notiType, usbNoti, device)) {
			privId = um_update_remove_notification(ad,
							notiType,
							usbNoti->privId,
							device);
			um_retvm_if(privId < 0, -1, "FAIL: um_update_notification()");
			*rmList = l;
			break;
		}
	}

	__USB_FUNC_EXIT__;
	return privId;
}

static int um_remove_cumulative_notification(UmMainData *ad, int notiType, void *device)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(device);

	int privId = -1;
	GList *rmList = NULL;

	privId = um_remove_cumulative_notification_host_acc(ad, notiType, device, &rmList);
	if (privId < 0 || !rmList) {
		USB_LOG("FAIL: um_remove_cumulative_notification_host_acc()");
		return -1;
	}

	switch (notiType) {
	case USB_NOTI_HOST_REMOVED:
		ad->notiHostList = g_list_delete_link(ad->notiHostList, rmList);
		break;
	case USB_NOTI_ACC_REMOVED:
		ad->notiAccList = g_list_delete_link(ad->notiAccList, rmList);
		break;
	default:
		break;
	}

	__USB_FUNC_EXIT__;
	return privId;
}

int um_remove_host_notification(UmMainData *ad, int notiType, void *device)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(device);

	int privId = -1;

	switch (notiType) {
	case USB_NOTI_MASS_STORAGE_REMOVED:
		privId = um_remove_separate_notification(ad, notiType, device);
		um_retvm_if(privId < 0, -1, "FAIL: um_remove_separate_notification()");
		break;
	case USB_NOTI_HOST_REMOVED:
		privId = um_remove_cumulative_notification(ad, notiType, device);
		um_retvm_if(privId < 0, -1, "FAIL: um_remove_cumulative_notification()");
		break;
	default:
		break;
	}
	__USB_FUNC_EXIT__;
	return 0;
}

int um_remove_client_notification(UmMainData *ad, int notiType, void *device)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);
	assert(device);

	if (notiType != USB_NOTI_ACC_REMOVED) {
		USB_LOG("ERROR: This function is just for usb accessory");
		return -1;
	}
	int privId = -1;

	privId = um_remove_cumulative_notification(ad, notiType, device);
	um_retvm_if(privId < 0, -1, "FAIL: um_remove_cumulative_notification()");
	__USB_FUNC_EXIT__;
	return 0;
}

int um_remove_all_client_notification(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);

	int cnt = -1;
	int i = -1;
	int ret = -1;
	UsbNotification *usbNoti = NULL;

	if (ad->notiAccList) {
		cnt = g_list_length(ad->notiAccList);
		for (i = 0 ; i < cnt ; i++) {
			usbNoti = (UsbNotification *)g_list_nth_data(ad->notiAccList, i);
			ret = um_remove_client_notification(ad, 0, usbNoti->accDev);
			if (ret < 0) USB_LOG("FAIL: um_remove_client_notification()");
		}
	}

	g_list_free_full(ad->notiAccList, release_usb_notification);

	__USB_FUNC_EXIT__;
	return 0;
}

int um_remove_all_host_notification(UmMainData *ad)
{
	__USB_FUNC_ENTER__ ;
	assert(ad);

	int cnt = -1;
	int i = -1;
	int ret = -1;
	UsbNotification *usbNoti = NULL;

	if (ad->notiHostList) {
		cnt = g_list_length(ad->notiHostList);
		for (i = 0 ; i < cnt ; i++) {
			usbNoti = (UsbNotification *)g_list_nth_data(ad->notiHostList, i);
			ret = um_remove_host_notification(ad, 0, usbNoti->devIf);
			if (ret < 0) USB_LOG("FAIL: um_remove_host_notification()");
		}
	}

	if (ad->notiMSList) {
		cnt = g_list_length(ad->notiMSList);
		for (i = 0 ; i < cnt ; i++) {
			usbNoti = (UsbNotification *)g_list_nth_data(ad->notiMSList, i);
			ret = um_remove_host_notification(ad,
							USB_HOST_MASS_STORAGE,
							usbNoti->devName);
			if (ret < 0) USB_LOG("FAIL: um_remove_host_notification()");
		}
	}

	g_list_free_full(ad->notiHostList, release_usb_notification);
	g_list_free_full(ad->notiMSList, release_usb_notification);

	__USB_FUNC_EXIT__;
	return 0;
}

