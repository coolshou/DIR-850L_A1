#!/bin/sh
echo [$0][$1][$2][$3][$4][$5][$6][$7] ... > /dev/console
xmldbc -P /etc/scripts/route_dynamic_helper.php -V DEST=$1 -V GATE=$2 -V MASK=$3 -V INF=$4 -V METRIC=$5 -V FAMILY=$6 -V TYPE=$7 >/var/run/route_dynamic_helper.sh
sh /var/run/route_dynamic_helper.sh > /dev/console