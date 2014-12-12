#!/bin/sh
BDNO=`cat rootfs/etc/config/buildno`
SIGN=`cat rootfs/etc/config/fw_sign`

mv $1 images/$2
echo -e "\033[32m-----------------------------------------------------\033[0m"
echo -e "\033[32mImage file   : $2\033[0m"
echo -e "\033[32mSignature    : $SIGN\033[0m"
echo -e "\033[32mBuild number : $BDNO\033[0m"
echo -e "\033[32m-----------------------------------------------------\033[0m"
ls -al images/$2
