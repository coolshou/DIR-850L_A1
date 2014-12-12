#!/bin/sh
BDNO=`cat rootfs/etc/config/buildno`
SIGN=`cat rootfs/etc/config/image_sign`

echo -e "\033[32m- TFTP image ----------------------------------------\033[0m"
echo -e "\033[32mImage file   : $1\033[0m"
echo -e "\033[32mSignature    : $SIGN\033[0m"
echo -e "\033[32mBuild number : $BDNO\033[0m"
echo -e "\033[32m-----------------------------------------------------\033[0m"
[ -f raw.img ] && mv raw.img $1
if [ ! -d /tftpboot/$USER ]; then
	mkdir -p /tftpboot/$USER
fi
cp -f $1 /tftpboot/$USER/.
ls -al $1
