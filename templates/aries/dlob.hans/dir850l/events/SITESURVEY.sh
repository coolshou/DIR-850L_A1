#!/bin/sh
iwlist wlan0 scanning > /var/ssvy.txt
Parse2DB sitesurvey -f /var/ssvy.txt -d > /dev/null
rm /var/ssvy.txt
exit 0
