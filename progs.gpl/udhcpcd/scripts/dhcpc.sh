#!/bin/sh
usage="Usage: dhcpc.sh {interface} {start|stop|status}"
[ -z "$1" ] && echo $usage && exit 1

SCRIPTFILE=/etc/scripts/udhcp/udhcpc.sh
PIDFILE=/var/run/udhcpc-$1.pid
DHCPC=udhcpc
HOST=`rgdb -g /sys/hostName`

case "$2" in
start)
	echo "Start DHCP client on $1 ..."
	if [ "$HOST" != "" ]; then
		HOST="-H $HOST"
	fi
	$DHCPC -i $1 -p $PIDFILE $HOST -s $SCRIPTFILE -D 2 -R 5 -S 300 &
	;;
stop)
	echo "Stop udhcpc on $1 ..."
	if [ -f $PIDFILE ]; then
		PID=`cat $PIDFILE`
		if [ $PID != 0 ]; then
			kill -9 $PID > /dev/null 2>&1
		fi
		rm -f $PIDFILE
	fi
	ifconfig $1 0.0.0.0 > /dev/null 2>&1
	;;
status)
	if [ -f $PIDFILE ]; then
		echo "enabled"
	else
		echo "disabled"
	fi
	;;
*)
	echo $usage
	exit 1
	;;
esac
exit 0
