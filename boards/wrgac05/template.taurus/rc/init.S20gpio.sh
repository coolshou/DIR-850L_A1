#!/bin/sh
insmod /lib/modules/gpio.ko
mknod /dev/gpio c 101 0 > /dev/null 2>&1 &

