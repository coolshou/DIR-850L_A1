#!/bin/sh
echo "aaaaaaaa" > /dev/console
xmldbc -P /etc/services/WIFI/hostapdcfg.php > /var/topology.conf
killall hostapd > /dev/null 2>&1
hostapd /var/topology.conf &
xmldbc -X /runtime/hostapd_restartap
