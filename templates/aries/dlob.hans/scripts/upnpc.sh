#!/bin/sh
echo [$0] $1 $2 $3 $4... > /dev/console
case "$1" in
add)
	service UPNPC restart
	;;
del)
	upnpc -m $1 -d $2 $3
	service UPNPC restart
	;;
*)
    echo "usage:"
    echo "upnpc.sh add"
    echo "upnpc.sh del $IP $PORT $PROTOCOL"
    exit 1
    ;;
esac
