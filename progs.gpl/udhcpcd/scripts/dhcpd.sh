#!/bin/sh
usage="Usage: dhcpd.sh [interface] [start|stop|restart|status|clear]"
[ -z "$1" ] && echo $usage && exit 9

DHCPD=/usr/sbin/udhcpd
CONFF=/var/etc/udhcpd-$1.conf
PIDFL=/var/run/udhcpd-$1.pid
LEASE=/var/etc/udhcpd-$1.lease

dsvr="/lan/dhcp/server/pool:1"
stat="$dsvr/staticdhcp"
dhcpatchpid="/var/run/dhcpatch.pid"


case "$2" in
clear)
	echo -n > $LEASE
	;;
start)
	echo "Start DHCP server on $1 ..."
	[ -f $LEASE ] || echo -n > $LEASE
	[ `rgdb -g /lan/dhcp/server/enable` = "0" ] && exit 0
	ipadd=`ifconfig $1 | scut -p "inet addr:"`
	nmask=`ifconfig $1 | scut -p "Mask:"`
	ipstr=`rgdb -g $dsvr/startIp`
	ipend=`rgdb -g $dsvr/endIp`
	dmain=`rgdb -g $dsvr/domain`
	wins0=`rgdb -g $dsvr/primaryWins`
	wins1=`rgdb -g $dsvr/secondaryWins`
	ltime=`rgdb -g $dsvr/leaseTime`
	[ "$ltime" = "" ] && ltime=86400
	[ "$dmain" = "" ] && dmain=`scut -p search /etc/resolv.conf`

	echo -n							>  $CONFF
	echo "start $ipstr"					>> $CONFF
	echo "end $ipend"					>> $CONFF
	echo "interface $1"					>> $CONFF
	echo "lease_file $LEASE" 				>> $CONFF
	echo "pidfile $PIDFL"					>> $CONFF
	echo "auto_time 10"					>> $CONFF
	echo "opt lease $ltime"					>> $CONFF
	echo "opt domain $dmain"				>> $CONFF

	[ "$nmask" != "" ] && echo "opt subnet $nmask"		>> $CONFF
	[ "$ipadd" != "" ] && echo "opt router $ipadd"		>> $CONFF
	[ "$wins0" != "" ] && echo "opt wins $wins0"		>> $CONFF
	[ "$wins1" != "" ] && echo "opt wins $wins1"		>> $CONFF
	
	if [ `rgdb -g /dnsrelay/mode` != "1" ]; then
		echo "opt dns $ipadd"				>> $CONFF
	else
		dnss=`scut -p nameserver /etc/resolv.conf`
		dns="no dns"
		for i in $dnss ; do
			echo "opt dns $i"			>> $CONFF
			dns=""
		done
		[ "$dns" = "no dns" ] && echo "opt dns $ipadd"	>> $CONFF
	fi

	if [ `rgdb -g $stat/enable` = "1" ]; then
		static=`rgdb -g $stat/entry#`
		i=1
		while [ $i -le $static ]
		do
			if [ `rgdb -g $stat/entry:"$i"/enable` = "1" ]; then
				mac=`rgdb -g $stat/entry:"$i"/mac`
				ipa=`rgdb -g $stat/entry:"$i"/ip`
				echo "static $ipa $mac" >> $CONFF
			fi
			i=`expr $i + 1`
		done
	fi

	$DHCPD $CONFF &
	dhcpxmlpatch -f $LEASE &
	echo $! > $dhcpatchpid
	;;
stop)
	if [ -f $dhcpatchpid ]; then
		pid=`cat $dhcpatchpid`
		if [ $pid != 0 ]; then
			kill $pid > /dev/null 2>&1
		fi
		rm -f $dhcpatchpid
	fi

	if [ -f $PIDFL ]; then
		PID=`cat $PIDFL`
		if [ $PID != 0 ]; then
			kill -9 $PID > /dev/null 2>&1
		fi
		rm -f $PIDFL
	fi
	;;
restart)
	echo "Restart DHCP server $1 ..."
	$0 $1 stop > /dev/null 2>&1
	$0 $1 start > /dev/null 2>&1
	;;
status)
	if [ -e $PIDFL ]; then
		echo "running"
	else
		echo "stop"
	fi
	;;
*)
	echo $usage
	exit 9
	;;
esac
exit 0
