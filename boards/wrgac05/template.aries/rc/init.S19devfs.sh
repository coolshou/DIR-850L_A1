#!/bin/sh
insmod /lib/modules/gpio.ko
mknod /dev/gpio c 101 0
