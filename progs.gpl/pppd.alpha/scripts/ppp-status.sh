#!/bin/sh
echo [$0]: linkname[$1] message[$2] session[$3] mtu[$4] ... > /dev/console
rgdb -i -s /runtime/wan/inf:1/connectStatus "$2"
