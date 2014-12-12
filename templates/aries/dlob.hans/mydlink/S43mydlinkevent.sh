#!/bin/sh
#this should be after smart 404(S41)...
routermode="`xmldbc -g /device/layout`"

if [ "$routermode" != "router" ] ; then
echo "not enable mydlink in NOT router mode"
exit 0
fi

echo [$0]: $1 ... > /dev/console
if [ "$1" = "start" ]; then
event WAN-1.UP	insert "mydlink:/mydlink/opt.local restart"
event WAN-1.DOWN	insert "mydlink:/mydlink/opt.local stop"
event WAN-1.UP	insert "checkfw:sh /etc/scripts/mydlink/fw_mail_check.sh"
event MYDLINK_TESTMAIL add "mdtestmail"
service MYDLINK.LOG $1
arpmonitor -i br0 &
else 
event WAN-1.UP	remove mydlink
event WAN-1.UP	remove checkfw
event WAN-1.DOWN	remove mydlink
event MYDLINK_TESTMAIL flush
service MYDLINK.LOG $1
fi
