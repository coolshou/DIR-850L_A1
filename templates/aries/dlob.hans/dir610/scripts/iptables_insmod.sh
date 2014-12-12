#!/bin/sh
# kernel > 2.6.22, it (may)will use nf_conntrack_max
CONNTRACK_MAX=`xmldbc -g /runtime/device/conntrack_max`
if [ "$CONNTRACK_MAX" = ""  ]; then
	CONNTRACK_MAX=8192
	echo "CONNTRACK_MAX=$CONNTRACK_MAX"
fi

if [ -f /proc/sys/net/netfilter/nf_conntrack_max ]; then
	echo $CONNTRACK_MAX > /proc/sys/net/netfilter/nf_conntrack_max
else
	echo $CONNTRACK_MAX > /proc/sys/net/ipv4/ip_conntrack_max
fi

# For non-NAPI dev(such as ralink wireless interface, may can smaller )
echo 100 > /proc/sys/net/core/netdev_max_backlog
# For NAPI dev(such as ralink ethernet interface, may can smaller )
echo 16 > /proc/sys/net/core/netdev_budget

echo 16 > /proc/sys/net/core/dev_weight

if [ -f /lib/modules/sw_tcpip.ko ]; then
	insmod /lib/modules/sw_tcpip.ko
fi

if [ -f /lib/modules/ifresetcnt.ko ]; then
	insmod /lib/modules/ifresetcnt.ko
fi
