#!/bin/sh
#echo [$0] [$1] [$2] [$3]... > /dev/console
xmldbc -P /etc/scripts/backupconn.php -V PHYINF=$1 -V INF=$2 -V CONN=$3 > /var/run/$2_backup_start.sh
sh /var/run/$2_backup_start.sh
