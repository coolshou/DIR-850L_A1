#!/bin/sh
image_sign=`cat /etc/config/image_sign`
mkdir -p /var/servd
xmldb -n $image_sign -t > /dev/console &
servd -d schedule_off > /dev/console 2>&1 &
sleep 1
/etc/scripts/dbload.sh
service LOGD start
echo "1" > /proc/sys/kernel/panic

#some firewall will block this
echo "0" > /proc/sys/net/ipv4/tcp_timestamps

# this mean dirty data will be write out after one sec.
# this can help write data to disk ASAP before user unplug usb drive.
# and no need to mount with sync option.
echo 100 > /proc/sys/vm/dirty_expire_centisecs

exit 0
