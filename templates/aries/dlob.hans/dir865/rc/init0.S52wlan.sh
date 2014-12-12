#!/bin/sh

xmldbc -P /etc/services/WIFI/rtcfg.php -V ACTION="INIT" > /var/init_wifi_mod.sh

# init wireless power 
nvram set pci/2/1/maxp2ga0=0x64
nvram set pci/2/1/maxp2ga1=0x64
nvram set pci/2/1/maxp2ga2=0x64
nvram set pci/2/1/cckbw202gpo=0x7777
nvram set pci/2/1/cckbw20ul2gpo=0x7777
nvram set pci/2/1/legofdmbw202gpo=0x77777777
nvram set pci/2/1/legofdmbw20ul2gpo=0x77777777
nvram set pci/2/1/mcsbw202gpo=0x77777777
nvram set pci/2/1/mcsbw20ul2gpo=0x77777777
nvram set pci/2/1/mcsbw402gpo=0x99999999

nvram set pci/2/1/pa2gw0a0=0xFE61
nvram set pci/2/1/pa2gw1a0=0x1DE5
nvram set pci/2/1/pa2gw2a0=0xF8B8
nvram set pci/2/1/pa2gw0a1=0xFE5F
nvram set pci/2/1/pa2gw1a1=0x1D25
nvram set pci/2/1/pa2gw2a1=0xF8DD
nvram set pci/2/1/pa2gw0a2=0xFE50
nvram set pci/2/1/pa2gw1a2=0x1CE8
nvram set pci/2/1/pa2gw2a2=0xF8D2

#we only insert wifi modules in init. 
xmldbc -P /etc/services/WIFI/init_wifi_mod.php >> /var/init_wifi_mod.sh
chmod +x /var/init_wifi_mod.sh
/bin/sh /var/init_wifi_mod.sh

#initial wifi interfaces
service PHYINF.WIFI start
