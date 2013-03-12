#!/bin/sh

if [ "$1" = "--set" ]
then
	vconftool set -t bool db/setting/debug_mode "1" -f
	echo "Debug mode is loaded"

elif [ "$1" = "--unset" ]
then
	vconftool set -t bool db/setting/debug_mode "0" -f
	echo "USB default mode is loaded"

elif [ "$1" = "--debug" ]
then
	vconftool set -t bool db/setting/debug_mode "1" -f
	echo "Debug mode is loaded"
	vconftool set -t int db/setting/lcd_backlight_normal "600" -f
	echo "The backlight time of the display is set to 10 minutes"
	vconftool set -t bool db/setting/brightness_automatic "1" -f
	echo "Brightness is set automatic"

elif [ "$1" = "--sshon" ]
then
	vconftool set -t int memory/setting/usb_sel_mode "3" -f
	vconftool set -t int db/usb/keep_ethernet "1" -f
	echo "SSH is enabled"

elif [ "$1" = "--sshoff" ]
then
	vconftool set -t int memory/setting/usb_sel_mode "0" -f
	vconftool set -t int db/usb/keep_ethernet "0" -f
	echo "SSH is disabled"


elif [ "$1" = "--help" ]
then
	echo "set_usb_debug.sh: usage:"
	echo "    --help      this message "
	echo "    --set       load Debug mode"
	echo "    --unset     unload Debug mode"
	echo "    --debug     load debug mode with 10 minutes backlight time and automatic brightness"
	echo "    --sshon     load SSH mode"
	echo "    --sshoff    unload SSH mode"

else
	echo "Wrong parameters. Please use option --help to check options "

fi
