#!/bin/sh

[ -z "$1" ] && echo "Error: should be called from udhcpc" && exit 1

. /etc/config/defines

echo "udhcpc.sh $1"

RESOLV_CONF="/etc/resolv.conf"
[ -n "$broadcast" ] && BROADCAST="broadcast $broadcast"
[ -n "$subnet" ] && NETMASK="netmask $subnet"
cfg=`cat $wan_config`

case "$1" in
deconfig)
	echo "deconfig $interface"
	case "$cfg" in
	l2tp)
		/etc/scripts/ppp/ppp.sh stop wan1
		;;
	bigpond)
		/etc/scripts/misc/bigpond.sh stop
		;;
	*)
		ifconfig $interface 0.0.0.0
		[ -f /etc/scripts/misc/wandown.sh ] && /etc/scripts/misc/wandown.sh
		;;
	esac
	ifconfig $interface 0.0.0.0
	;;

renew|bound)
	echo "config $interface $ip $BROADCAST $NETMASK"
	ifconfig $interface $ip $BROADCAST $NETMASK
	if [ -n "$router" ] ; then
		echo "deleting routers"
		while route del default gw 0.0.0.0 dev $interface ; do
			:
		done
		for i in $router ; do
			route add default gw $i dev $interface
		done
	fi

	echo -n > $RESOLV_CONF
	DNS=`rgdb -g /dnsRelay/server/primarydns`
	[ "$DNS" != "" -a "$DNS" != "0.0.0.0" ] && echo nameserver $DNS >> $RESOLV_CONF
	DNS=`rgdb -g /dnsRelay/server/secondarydns`
	[ "$DNS" != "" -a "$DNS" != "0.0.0.0" ] && echo nameserver $DNS >> $RESOLV_CONF

	[ -n "$domain" ] && echo search $domain >> $RESOLV_CONF
	for i in $dns ; do
		echo adding dns $i
		echo nameserver $i >> $RESOLV_CONF
	done

	case "$cfg" in
	l2tp)
		/etc/scripts/ppp/ppp-setup.sh l2tp wan1
		/etc/scripts/ppp/ppp.sh start wan1
		;;
	bigpond)
		/etc/scripts/misc/bigpond.sh start
		;;
	*)
		[ -f /etc/scripts/misc/wanup.sh ] && /etc/scripts/misc/wanup.sh
		;;
	esac
	;;
esac
exit 0
