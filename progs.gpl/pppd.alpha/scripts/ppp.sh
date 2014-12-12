#!/bin/sh
usage="Usage: pppd.sh [start|stop|state|status] [linkname]"
case "$1" in
start|stop|state|status)
	;;
*)
	echo $usage
	;;
esac
[ "$2" = "" ] && echo $usage && exit 9

OPTS="/etc/ppp/options.$2"
LOOPIDF="/var/run/ppp-loop-$2.pid"
PIDF="/var/run/ppp-$2.pid"

case "$1" in
state)
	if [ -f "$LOOPIDF" ]; then
		pid=`cat $LOOPIDF`
		psl=`ps | scut -p $pid -n3`
		if [ "$psl" != "" ]; then
			echo "enabled"
			exit 0
		fi
	fi
	echo "disabled"
	exit 1
	;;
status)
	if [ -f "$PIDF" ]; then
		DEMD=`grep demand $OPTS`
		if [ "$DEMD" = "demand" ]; then
			echo "demand"
			exit 0
		fi
		echo "up"
	else
		echo "down"
	fi
	exit 0
	;;
stop)
	if [ -f "$LOOPIDF" ]; then
		pid=`cat $LOOPIDF`
		if [ "$pid" != "0" ]; then
			kill $pid > /dev/null 2>&1
		fi
		rm -f $LOOPIDF
	fi
	if [ -f "$PIDF" ]; then
		pid=`pfile -f $PIDF`
		if [ "$pid" != "0" ]; then
			kill $pid > /dev/null 2>&1
		fi
		rm -f $PIDF
	fi
	exit 0
	;;
start)
	/etc/scripts/ppp/ppp-loop.sh $2 &
	echo $! > $LOOPIDF
	;;
esac
exit 0
