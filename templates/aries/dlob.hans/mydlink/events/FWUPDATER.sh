#!/bin/sh
echo "[$0] ..."
# Copy "/bin/echo" command to /var/. (note: /var is ramfs) to avoid system cann't reboot issue
echo "kill servd and xmldb load.sh"
killall -9 servd
killall -9 xmldb
mkdir /var/bin

echo "stop mydlink"
/mydlink/opt.local stop
echo "umount /mydlink"
umount /mydlink -l

LOADSH=`ps | grep "cpuload.sh" | grep -v "grep"`
if [ -n "$LOADSH" ]; then
LOADSHPID=`echo $LOADSH | cut -d" " -f1`
kill $LOADSHPID
fi
export PATH=/var/bin:$PATH

echo "copy upgrade needed file to ramfs"
if [ -f /bin/busybox ]; then
	cp -f /bin/busybox /var/bin
	ln -s ./busybox /var/bin/echo
	ln -s ./busybox /var/bin/sh
	ln -s ./busybox /var/bin/mount
	ln -s ./busybox /var/bin/umount
	ln -s ./busybox /var/bin/reboot
fi
if [ -f /bin/busybox ]; then
cp -f /usr/sbin/fwupdater /var/bin
fi
if [ $? -eq 0 ]; then 
# remove any rootfs file
echo "remout /www /htdocs /etc /usr /bin /sbin to empty ramfs avoid someone access rootfs"
mount -t ramfs ramfs /etc
mount -t ramfs ramfs /htdocs
mount -t ramfs ramfs /www
mount -t ramfs ramfs /usr
mount -t ramfs ramfs /bin
mount -t ramfs ramfs /sbin
fi
fwupdater -i /var/firmware.seama
echo 1 > /proc/driver/system_reset
echo 1 > /proc/system_reset
#broadcom reboot using busybox
reboot
