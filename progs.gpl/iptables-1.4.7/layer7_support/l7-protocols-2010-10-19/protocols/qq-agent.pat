# HTTP agent for QQ
# Tencent QQ Protocol - Chinese instant messenger protocol - http://www.qq.com
# Pattern attributes: good fast
# Protocol groups: chat
# Wiki: http://www.protocolinfo.org/wiki/QQ
#
# Over six million people use QQ in China, according to wsgtrsys.
# 
# This pattern has been tested and is believed to work well.
#
# kwest_wan@cn.alphanetworks.com
# 
# QQ use three connection method to connect to it's server.
# 1. Direct connect.
# 2. HTTP proxy.
# 3. Sock5 proxy.
# 
# This pat is against 2nd method.
#
qq-agent
^\x43\x4f\x4e\x4e\x45\x43\x54.+\x0d\x0a$
