#!/bin/sh
OPTS="/etc/ppp/options.$1"
hostname=`rgdb -g /sys/hostname`
case "$hostname" in
DAP-3520)
        type=`rgdb -g /wan_apc/rg/inf:1/mode`
	WANIF="/wan_apc/rg/inf:1"
	;;
*)
        type=`rgdb -g /wan/rg/inf:1/mode`
	WANIF="/wan/rg/inf:1"
	;;	
esac

case $type in
"3")
	AUTO=`rgdb -g $WANIF/pppoe/autoReconnect`
	;;
"4")
	AUTO=`rgdb -g $WANIF/pptp/autoReconnect`
	;;
"5")
	AUTO=`rgdb -g $WANIF/l2tp/autoReconnect`
	;;
esac

pppd file $OPTS
while [ ${AUTO} = "1" ]; do
	pppd file $OPTS
done
