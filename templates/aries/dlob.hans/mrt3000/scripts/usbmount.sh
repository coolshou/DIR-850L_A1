#!/bin/sh
echo [$0] $1 $2 ... > /dev/console

case "$1" in
add)
	/usr/sbin/usbmount $1 $2
	if [ -f /sys/block/$2/queue/nr_requests ]; then
		echo "64" > /sys/block/$2/queue/nr_requests
		echo "512" > /sys/block/$2/queue/read_ahead_kb
	fi
    ;;
remove)
	/usr/sbin/usbmount $1 $2
	xmldbc -s /upnpav/dms/sharepath ""
	xmldbc -s /itunes/server/sharepath ""
	;;
*)
	echo "not support [$1] ..."
    ;;
esac

exit 0


