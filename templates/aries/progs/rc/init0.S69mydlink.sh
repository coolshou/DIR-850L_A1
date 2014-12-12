#!/bin/sh
echo [$0]: $1 ... > /dev/console
DEVMYDLINK=`cat /etc/config/mydlink`
if [ "$1" = "start" ]; then
	mount -t squashfs $DEVMYDLINK /mydlink
	if [ $? = 0 ]; then
	echo "Use mydlink engine in $DEVMYDLINK!!"
	else
	echo "Use mydlink engine in rootfs!!"
	fi
fi
