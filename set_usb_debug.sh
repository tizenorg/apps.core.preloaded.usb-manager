#!/bin/sh

case "$1" in

"--debug")
	vconftool set -t int db/usb/sel_mode "2" -f
	vconftool set -t int db/setting/lcd_backlight_normal "600" -f
	echo "The backlight time of the display is set to 10 minutes"
	vconftool set -t bool db/setting/brightness_automatic "1" -f
	echo "Brightness is set automatic"
	;;

"--default" | "--sshoff" | "--unset")
	vconftool set -t int db/usb/sel_mode "1" -f
	vconftool set -t bool db/setting/debug_mode "0" -f
	;;

"--sdb" | "--set")
	vconftool set -t int db/usb/sel_mode "2" -f
	vconftool set -t bool db/setting/debug_mode "1" -f
	;;

"--sdb-diag")
	vconftool set -t int db/usb/sel_mode "3" -f
	vconftool set -t bool db/setting/debug_mode "1" -f
	;;

"--rndis-tethering")
	vconftool set -t int db/usb/sel_mode "4" -f
	;;

"--rndis" | "--sshon")
	vconftool set -t int db/usb/sel_mode "5" -f
	vconftool set -t bool db/setting/debug_mode "0" -f
	;;

"--rndis-sdb")
	vconftool set -t int db/usb/sel_mode "6" -f
	vconftool set -t bool db/setting/debug_mode "1" -f
	;;

"--help")
	echo "set_usb_debug.sh: usage:"
	echo "    --help           This message "
	echo "    --set            Load Debug mode"
	echo "    --unset          Unload Debug mode"
	echo "    --debug          Load debug mode with 10 minutes backlight time"
	echo "                     and automatic brightness"
	echo "    --sshon          Load SSH mode"
	echo "    --sshoff         Unload SSH mode"
	echo "    --default        Charging mode"
	echo "    --sdb            Load sdb"
	echo "    --sdb-diag       Load sdb, and diag"
	echo "                     If diag is not supported, charging mode is loaded"
	echo "    --rndis          Load rndis only"
	;;

*)
	echo "Wrong parameters. Please use option --help to check options "
	;;

esac
