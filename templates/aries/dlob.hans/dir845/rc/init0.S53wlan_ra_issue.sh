#!/bin/sh

host24=`xmldbc -w /phyinf:4/active`
guest24=`xmldbc -w /phyinf:5/active`

# if 2.4g will be enabled, we do nothing
[ "$host24" != "0" -o "$guest24" != "0" ] && exit 0

# trigger 2.4g interface up
echo "Hey! this is a workaround! Remove this if ralink issue is fixed!"
echo "Enable 2.4g interface!"

echo "Default" > /var/run/RT2860.dat
echo "CountryRegion=0" >> /var/run/RT2860.dat
echo "BasicRate=15" >> /var/run/RT2860.dat
echo "MacAddress=00:be:ef:ca:fe:34" >> /var/run/RT2860.dat
echo "CountryCode=US" >> /var/run/RT2860.dat
echo "WirelessMode=9" >> /var/run/RT2860.dat
echo "WmmCapable=1" >> /var/run/RT2860.dat
echo "APSDCapable=1" >> /var/run/RT2860.dat
echo "TxBurst=0" >> /var/run/RT2860.dat
echo "HT_TxStream=2" >> /var/run/RT2860.dat
echo "HT_RxStream=2" >> /var/run/RT2860.dat
echo "APAifsn=3;7;1;1" >> /var/run/RT2860.dat
echo "APCwmin=4;4;3;2" >> /var/run/RT2860.dat
echo "APCwmax=6;10;4;3" >> /var/run/RT2860.dat
echo "APTxop=0;0;94;47" >> /var/run/RT2860.dat
echo "APACM=0;0;0;0" >> /var/run/RT2860.dat
echo "BSSAifsn=3;7;2;2" >> /var/run/RT2860.dat
echo "BSSCwmin=4;4;3;2" >> /var/run/RT2860.dat
echo "BSSCwmax=10;10;4;3" >> /var/run/RT2860.dat
echo "BSSTxop=0;0;94;47" >> /var/run/RT2860.dat
echo "BSSACM=0;0;0;0" >> /var/run/RT2860.dat
echo "AckPolicy=0;0;0;0" >> /var/run/RT2860.dat
echo "BssidNum=2" >> /var/run/RT2860.dat
echo "SSID1=DUMMY" >> /var/run/RT2860.dat
echo "AccessPolicy0=0" >> /var/run/RT2860.dat
echo "SSID2=DUMMY" >> /var/run/RT2860.dat
echo "AccessPolicy1=0" >> /var/run/RT2860.dat
echo "HideSSID=0;0" >> /var/run/RT2860.dat
echo "AuthMode=OPEN;OPEN" >> /var/run/RT2860.dat
echo "EncrypType=NONE;NONE" >> /var/run/RT2860.dat
echo "AutoChannelSelect=0" >> /var/run/RT2860.dat
echo "BeaconPeriod=100" >> /var/run/RT2860.dat
echo "DtimPeriod=1" >> /var/run/RT2860.dat
echo "RTSThreshold=2346" >> /var/run/RT2860.dat
echo "FragThreshold=2346" >> /var/run/RT2860.dat
echo "TxPower=95" >> /var/run/RT2860.dat
echo "TxPreamble=0" >> /var/run/RT2860.dat
echo "WiFiTest=1" >> /var/run/RT2860.dat
echo "ShortSlot=1" >> /var/run/RT2860.dat
echo "CSPeriod=6" >> /var/run/RT2860.dat
echo "PktAggregate=1" >> /var/run/RT2860.dat
echo "HT_MCS=33" >> /var/run/RT2860.dat
echo "HT_HTC=1" >> /var/run/RT2860.dat
echo "HT_RDG=1" >> /var/run/RT2860.dat
echo "HT_LinkAdapt=0" >> /var/run/RT2860.dat
echo "HT_OpMode=0" >> /var/run/RT2860.dat
echo "HT_MpduDensity=5" >> /var/run/RT2860.dat
echo "HT_AutoBA=1" >> /var/run/RT2860.dat
echo "HT_AMSDU=0" >> /var/run/RT2860.dat
echo "HT_BAWinSize=64" >> /var/run/RT2860.dat
echo "HT_STBC=1" >> /var/run/RT2860.dat
echo "HT_BADecline=0" >> /var/run/RT2860.dat
echo "HT_PROTECT=1" >> /var/run/RT2860.dat
echo "HT_GI=1" >> /var/run/RT2860.dat
echo "HT_BW=1" >> /var/run/RT2860.dat
echo "HT_BSSCoexistence=0" >> /var/run/RT2860.dat
echo "ChannelGeography=1" >> /var/run/RT2860.dat
echo "HT_EXTCHA=1" >> /var/run/RT2860.dat
echo "NoForwarding=0;0" >> /var/run/RT2860.dat
echo "NoForwardingBTNBSSID=1" >> /var/run/RT2860.dat
echo "AntGain=4" >> /var/run/RT2860.dat
echo "BandedgeDelta=0" >> /var/run/RT2860.dat

ifconfig wifig0 up
iwpriv wifig0 set RadioOn=0

echo "Disable 2.4g interface!"

ifconfig wifig0 down
rm /var/run/RT2860.dat
