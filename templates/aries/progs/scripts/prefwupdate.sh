#!/bin/sh
echo "[$0] ...." > /dev/console
service WAN stop
service WIFI stop
service PHYINF.WIFI stop
service LAN stop
service UPNPAV stop
service ITUNES stop
service WEBACCESS stop
service SAMBA stop
event STATUS.CRITICAL
sleep 3
event HTTP.DOWN add /etc/events/FWUPDATER.sh
service HTTP stop
exit 0
