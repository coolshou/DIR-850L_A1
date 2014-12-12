#!/bin/sh
# dump the DOM tree to a file.

#patch for 404
orig_devconfsize=`xmldbc -g /runtime/device/devconfsize`

xmldbc -d /var/config.xml
gzip /var/config.xml
devconf put -f /var/config.xml.gz
rm -f /var/config.xml.gz

if [ "$orig_devconfsize" = "0" ]; then
	xmldbc -t "update404:3:phpsh /etc/events/update_smart404.php"
fi
