#! /bin/sh
LIB_PATH=""
HEADER_PATH=""

cd ..
FF_PATH=$PWD
echo $FF_PATH
cd mt-daapd-svn-1696

LIB_PATH="$FF_PATH/lib"
echo "LIB_PATH=$LIB_PATH"

HEADER_PATH="$FF_PATH/include"
echo "HEADER_PATH=$HEADER_PATH"

./configure --host=arm-linux --prefix=/usr/local/firefly --enable-sqlite3  --enable-iconv \
--with-sqlite3-libs=$LIB_PATH \
--with-sqlite3-includes=$HEADER_PATH 
#--with-id3tag=$FF_PATH


