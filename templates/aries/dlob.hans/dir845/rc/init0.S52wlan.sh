#!/bin/sh

#we only insert wifi modules in init. 
xmldbc -P /etc/services/WIFI/init_wifi_mod.php > /var/init_wifi_mod.sh
chmod +x /var/init_wifi_mod.sh
/bin/sh /var/init_wifi_mod.sh
