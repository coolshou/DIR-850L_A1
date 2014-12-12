#!/bin/sh
# dump the DOM tree to a file.
orig_devconfsize=`xmldbc -g /runtime/device/devconfsize`

xmldbc -d /var/config.xml
gzip /var/config.xml
devconf put -f /var/config.xml.gz
rm -f /var/config.xml.gz

if [ "$orig_devconfsize" = "0" ]; then
	killall telnetd
fi
