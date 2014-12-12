#!/bin/sh
usage="Usage: ppp-setup.sh [pppoe|pptp|l2tp] [linkname] [sync]"
case "$1" in
pppoe|pptp|l2tp)
	;;
*)
	echo $usage
	exit 9
	;;
esac
[ "$2" = "" ] && echo $usage && exit 9
echo "setup ppp options for $1 name:$2 ..."

. /etc/config/defines
OPTS="/etc/ppp/options.$2"

IF=`scut -p WANIF= $layout_config`

# XML nodes
hostname=`rgdb -g /sys/hostname`
case "$hostname" in
DAP-3520)
	wppoe="/rg/inf:1/pppoe"
	wpptp="/wan_apc/rg/inf:1/pptp"
	wl2tp="/wan_apc/rg/inf:1/l2tp"
	dnssv="/dnsRelay/server"
	;;
*)
	wppoe="/rg/inf:1/pppoe"
wpptp="/wan/rg/inf:1/pptp"
wl2tp="/wan/rg/inf:1/l2tp"
dnssv="/dnsRelay/server"
	;;	
esac

# Get configuration
case "$1" in
pppoe)
	MODE=`rgdb -g $wppoe/mode`
	USER=`rgdb -g $wppoe/user`
	PASS=`rgdb -g $wppoe/password`
	ACNM=`rgdb -g $wppoe/acName`
	SRVC=`rgdb -g $wppoe/acService`
	AUTO=`rgdb -g $wppoe/autoReconnect`
	DEMD=`rgdb -g $wppoe/onDemand`
	IDLE=`rgdb -g $wppoe/idleTimeout`
	ADNS=`rgdb -g $wppoe/autoDns`
	 MTU=`rgdb -g $wppoe/mtu`
	CMAC=`rgdb -g $wppoe/cloneMac`
	DNS1=`rgdb -g $dnssv/primaryDns`
	DNS2=`rgdb -g $dnssv/secondaryDns`
	LCIP=""

	[ "$ACNM" != "" ] && ACNM="pppoe_ac_name $ACNM"
	[ "$SRVC" != "" ] && SRVC="pppoe_srv_name $SRVC"
	[ "$MODE" = "1" ] && LCIP=`rgdb -g $wppoe/staticIp`
	;;
pptp)
	USER=`rgdb -g $wpptp/user`
	PASS=`rgdb -g $wpptp/password`
	AUTO=`rgdb -g $wpptp/autoReconnect`
	DEMD=`rgdb -g $wpptp/onDemand`
	IDLE=`rgdb -g $wpptp/idleTimeout`
	 MTU=`rgdb -g $wpptp/mtu`
	SVER=`rgdb -g $wpptp/serverIp`
	ADNS="1"
	LCIP=""
	;;
l2tp)
	USER=`rgdb -g $wl2tp/user`
	PASS=`rgdb -g $wl2tp/password`
	AUTO=`rgdb -g $wl2tp/autoReconnect`
	DEMD=`rgdb -g $wl2tp/onDemand`
	IDLE=`rgdb -g $wl2tp/idleTimeout`
	 MTU=`rgdb -g $wl2tp/mtu`
	SVER=`rgdb -g $wl2tp/serverIp`
	ADNS="1"
	LCIP=""
	;;
esac

# Use peer dns or not
UPDNS="usepeerdns"
[ "$ADNS" = "0" ] && UPDNS=""
if [ "$MTU" != "" ]; then
	MRU="mru $MTU"
	MTU="mtu $MTU"
else
	MRU="mru 1492"
	MTU="mtu 1492"
fi

# Generate authentication info
[ -f /etc/ppp/chap-secrets ] && rm -f /etc/ppp/chap-secrets
[ -f /etc/ppp/pap-secrets ] && rm -f /etc/ppp/pap-secrets
echo -n > /etc/ppp/pap-secrets
echo "\"$USER\" * \"$PASS\" *" >> /etc/ppp/pap-secrets
ln -s /etc/ppp/pap-secrets /etc/ppp/chap-secrets

# Prepare ip-up ip-down
cp /etc/scripts/ppp/conf/ip-up /etc/ppp/.
cp /etc/scripts/ppp/conf/ip-down /etc/ppp/.

# Generate options file
echo -n > $OPTS
[ "$4" = "debug" ] && echo "debug dump logfd 2" >> $OPTS
echo "noauth nodeflate nobsdcomp nodetach defaultroute" >> $OPTS
echo "linkname $2" >> $OPTS
echo "lcp-echo-failure 1" >> $OPTS
echo "lcp-echo-interval 30" >> $OPTS
echo "lcp-echo-failure-2 6" >> $OPTS
echo "lcp-echo-interval-2 10" >> $OPTS
echo "lcp-timeout-1 10" >> $OPTS
echo "lcp-timeout-2 10" >> $OPTS
echo "ipcp-accept-remote ipcp-accept-local" >> $OPTS
echo "$MTU $MRU" >> $OPTS
echo "user '$USER'" >> $OPTS
#echo "hide-password" >> $OPTS

# setup idle time (if enabled)
[ "$IDLE" != "" -a "$IDLE" != "0" ] && echo "idle $IDLE" >> $OPTS

# setup dial on demand (if enable)
if [ "$DEMD" = "1" ]; then
	echo "demand" >> $OPTS
	echo "connect true" >> $OPTS
	echo "ktune" >> $OPTS
	echo "noipdefault" >> $OPTS
	[ "$LCIP" = "" -o "$LCIP" = "0.0.0.0" ] && LCIP="10.112.112.112"
fi

# persist ?
if [ "$AUTO" = "1" ]; then
	echo "persist" >> $OPTS
	echo "maxfail 5" >> $OPTS
fi

# have default ip ?
if [ "$LCIP" != "" -a "$LCIP" != "0.0.0.0" ]; then
	echo "$LCIP:10.112.112.113" >> $OPTS
else
	echo "noipdefault" >> $OPTS
fi

#if [ "$AUTO" = "1" ]; then
#	echo "persist" >> $OPTS
#	echo "maxfail 0" >> $OPTS
#	if [ "$LCIP" != "" -a "$LCIP" != "0.0.0.0" ]; then
#		echo "$LCIP:10.112.112.113" >> $OPTS
#	else
#		echo "noipdefault" >> $OPTS
#	fi
#else
#	if [ "$DEMD" = "1" ]; then
#		echo "demand" >> $OPTS
#		echo "connect true" >> $OPTS
#		echo "ktune" >> $OPTS
#		echo "persist" >> $OPTS
#		echo "maxfail 0" >> $OPTS
#		if [ "$LCIP" != "" -a "$LCIP" != "0.0.0.0" ]; then
#			echo "$LCIP:10.112.112.113" >> $OPTS
#		else
#			echo "10.112.112.112:10.112.112.113" >> $OPTS
#			echo "noipdefault" >> $OPTS
#		fi
#	else
#		if [ "$LCIP" != "" -a "$LCIP" != "0.0.0.0" ]; then
#			echo "$LCIP:10.112.112.113" >> $OPTS
#		else
#			echo "noipdefault" >> $OPTS
#		fi
#	fi
#	[ "$IDLE" != "" -a "$IDLE" != "0" ] && echo "idle $IDLE" >> $OPTS
#fi


# Setup resolv.conf 
if [ "$UPDNS" != "" ]; then
	echo $UPDNS >> $OPTS
	rm -f /var/etc/resolv.conf
	ln -s /etc/ppp/resolv.conf /var/etc/resolv.conf
else
	echo -n > /var/etc/resolv.conf
	[ "$DNS1" != "" -a "$DNS1" != "0.0.0.0" ] && echo "nameserver $DNS1" >> /var/etc/resolv.conf
	[ "$DNS2" != "" -a "$DNS2" != "0.0.0.0" ] && echo "nameserver $DNS2" >> /var/etc/resolv.conf
fi

# Setup pty module
case "$1" in
pppoe)
	#echo "pty_pppoe pppoe_device $IF pppoe_mss 1400 $ACNM $SRVC" >> $OPTS
	#[ "$3" = "sync" ] && echo "sync pppoe_sync" >> $OPTS
	echo "kpppoe pppoe_device $IF $ACNM $SRVC" >> $OPTS
	;;
pptp)
	echo "pty_pptp pptp_server_ip $SVER" >> $OPTS
	echo "name '$USER'" >> $OPTS
	echo "refuse-eap" >> $OPTS
	[ "$3" = "sync" ] && echo "sync pptp_sync" >> $OPTS
	;;
l2tp)
	echo "pty_l2tp l2tp_peer $SVER" >> $OPTS
	[ "$3" = "sync" ] && echo "sync l2tp_sync" >> $OPTS
	;;
esac
exit 0
