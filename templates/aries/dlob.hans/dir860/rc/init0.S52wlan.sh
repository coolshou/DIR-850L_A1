#!/bin/sh

xmldbc -P /etc/services/WIFI/rtcfg.php -V ACTION="INIT" > /var/init_wifi_mod.sh

# set initial arguments for module 4331 (it has no srom values)
# more detail to check bcm94708_alpha_wrgac02.txt in bootcode
for slotidx in 0 1
do
nvram set pci/1/$slotidx/vendid=0x14E4
nvram set pci/1/$slotidx/maxp2ga0=0x50
nvram set pci/1/$slotidx/maxp2ga1=0x50
nvram set pci/1/$slotidx/cddpo=0x0
nvram set pci/1/$slotidx/cck2gpo=0x1111
nvram set pci/1/$slotidx/ofdm2gpo=0x77777777
nvram set pci/1/$slotidx/mcs2gpo0=0x7777
nvram set pci/1/$slotidx/mcs2gpo1=0x7777
nvram set pci/1/$slotidx/mcs2gpo2=0x7777
nvram set pci/1/$slotidx/mcs2gpo3=0x7777
nvram set pci/1/$slotidx/mcs2gpo4=0x7777
nvram set pci/1/$slotidx/mcs2gpo5=0x7777
nvram set pci/1/$slotidx/mcs2gpo6=0x7777
nvram set pci/1/$slotidx/mcs2gpo7=0x7777
nvram set pci/1/$slotidx/pa2gw0a0=0xFE9B
nvram set pci/1/$slotidx/pa2gw1a0=0x18C0
nvram set pci/1/$slotidx/pa2gw2a0=0xFA27
nvram set pci/1/$slotidx/pa2gw0a1=0xFEB3
nvram set pci/1/$slotidx/pa2gw1a1=0x18C4
nvram set pci/1/$slotidx/pa2gw2a1=0xFA4B
done

# 5G
for slotidx in 0 1
do
nvram set pci/2/$slotidx/sromrev=11
nvram set pci/2/$slotidx/venid=0x14E4
nvram set pci/2/$slotidx/vendid=0x14E4
nvram set pci/2/$slotidx/boardvendor=0x14E4
nvram set pci/2/$slotidx/devid=0x43b3
nvram set pci/2/$slotidx/boardrev=0x1350
nvram set pci/2/$slotidx/boardflags=0x10001000
nvram set pci/2/$slotidx/boardflags2=0x2
nvram set pci/2/$slotidx/boardtype=0x62f
nvram set pci/2/$slotidx/boardflags3=0x0
nvram set pci/2/$slotidx/boardnum=0
nvram set pci/2/$slotidx/macaddr=00:90:4c:d4:00:00
nvram set pci/2/$slotidx/ccode=0
nvram set pci/2/$slotidx/regrev=0
nvram set pci/2/$slotidx/aa2g=0
nvram set pci/2/$slotidx/aa5g=3
nvram set pci/2/$slotidx/agbg0=71
nvram set pci/2/$slotidx/agbg1=71
nvram set pci/2/$slotidx/agbg2=133
nvram set pci/2/$slotidx/aga0=71
nvram set pci/2/$slotidx/aga1=133
nvram set pci/2/$slotidx/aga2=133
nvram set pci/2/$slotidx/txchain=3
nvram set pci/2/$slotidx/rxchain=3
nvram set pci/2/$slotidx/antswitch=0
nvram set pci/2/$slotidx/tssiposslope2g=1
nvram set pci/2/$slotidx/epagain2g=0
nvram set pci/2/$slotidx/pdgain2g=10
nvram set pci/2/$slotidx/tworangetssi2g=0
nvram set pci/2/$slotidx/papdcap2g=0
nvram set pci/2/$slotidx/femctrl=6
nvram set pci/2/$slotidx/tssiposslope5g=1
nvram set pci/2/$slotidx/epagain5g=0
nvram set pci/2/$slotidx/pdgain5g=10
nvram set pci/2/$slotidx/tworangetssi5g=0
nvram set pci/2/$slotidx/papdcap5g=0
nvram set pci/2/$slotidx/gainctrlsph=0
nvram set pci/2/$slotidx/tempthresh=255
nvram set pci/2/$slotidx/tempoffset=255
nvram set pci/2/$slotidx/rawtempsense=0x1ff
nvram set pci/2/$slotidx/measpower=0x7f
nvram set pci/2/$slotidx/tempsense_slope=0xff
nvram set pci/2/$slotidx/tempcorrx=0x3f
nvram set pci/2/$slotidx/tempsense_option=0x3
nvram set pci/2/$slotidx/phycal_tempdelta=255
nvram set pci/2/$slotidx/temps_period=15
nvram set pci/2/$slotidx/temps_hysteresis=15
nvram set pci/2/$slotidx/measpower1=0x7f
nvram set pci/2/$slotidx/measpower2=0x7f
nvram set pci/2/$slotidx/pdoffset40ma0=12834
nvram set pci/2/$slotidx/pdoffset40ma1=12834
nvram set pci/2/$slotidx/pdoffset40ma2=12834
nvram set pci/2/$slotidx/pdoffset80ma0=256
nvram set pci/2/$slotidx/pdoffset80ma1=256
nvram set pci/2/$slotidx/pdoffset80ma2=256
nvram set pci/2/$slotidx/subband5gver=0x4
nvram set pci/2/$slotidx/maxp2ga0=66
nvram set pci/2/$slotidx/pa2ga0=0xff24,0x188e,0xfce6
nvram set pci/2/$slotidx/rxgains5gmelnagaina=7
nvram set pci/2/$slotidx/rxgains5gmtrisoa0=15
nvram set pci/2/$slotidx/rxgains5gmtrelnabyp=1
nvram set pci/2/$slotidx/rxgains5ghelnagaina=7
nvram set pci/2/$slotidx/rxgains5ghtrisoa0=15
nvram set pci/2/$slotidx/rxgains5ghtrelnabyp=1
nvram set pci/2/$slotidx/rxgains2gelnagaina0=0
nvram set pci/2/$slotidx/rxgains2gtrisoa0=0
nvram set pci/2/$slotidx/rxgains2gtrelnabypa=0
nvram set pci/2/$slotidx/rxgains5gelnagaina0=3
nvram set pci/2/$slotidx/rxgains5gtrisoa0=6
nvram set pci/2/$slotidx/rxgains5gtrelnabypa=1
nvram set pci/2/$slotidx/maxp5ga0=72,72,72,72
nvram set pci/2/$slotidx/maxp2ga1=66
nvram set pci/2/$slotidx/pa5ga0=0xff72,0x17d1,0xfd29,0xff78,0x183b,0xfd27,0xff75,0x1866,0xfd20,0xff85,0x18c8,0xfd30
nvram set pci/2/$slotidx/pa2ga1=0xff3e,0x15f9,0xfd36
nvram set pci/2/$slotidx/rxgains5gmelnagaina=7
nvram set pci/2/$slotidx/rxgains5gmtrisoa1=15
nvram set pci/2/$slotidx/rxgains5gmtrelnabyp=1
nvram set pci/2/$slotidx/rxgains5ghelnagaina=7
nvram set pci/2/$slotidx/rxgains5ghtrisoa1=15
nvram set pci/2/$slotidx/rxgains5ghtrelnabyp=1
nvram set pci/2/$slotidx/rxgains2gelnagaina1=0
nvram set pci/2/$slotidx/rxgains2gtrisoa1=0
nvram set pci/2/$slotidx/rxgains2gtrelnabypa=0
nvram set pci/2/$slotidx/rxgains5gelnagaina1=3
nvram set pci/2/$slotidx/rxgains5gtrisoa1=6
nvram set pci/2/$slotidx/rxgains5gtrelnabypa=1
nvram set pci/2/$slotidx/maxp5ga1=72,72,72,72
nvram set pci/2/$slotidx/pa5ga1=0xff4e,0x1593,0xfd4b,0xff61,0x1743,0xfd21,0xff6a,0x1721,0xfd41,0xff99,0x18e6,0xfd40
nvram set pci/2/$slotidx/maxp2ga2=66
nvram set pci/2/$slotidx/pa2ga2=0xff25,0x18a6,0xfce2
nvram set pci/2/$slotidx/rxgains5gmelnagaina=7
nvram set pci/2/$slotidx/rxgains5gmtrisoa2=15
nvram set pci/2/$slotidx/rxgains5gmtrelnabyp=1
nvram set pci/2/$slotidx/rxgains5ghelnagaina=7
nvram set pci/2/$slotidx/rxgains5ghtrisoa2=15
nvram set pci/2/$slotidx/rxgains5ghtrelnabyp=1
nvram set pci/2/$slotidx/rxgains2gelnagaina2=0
nvram set pci/2/$slotidx/rxgains2gtrisoa2=0
nvram set pci/2/$slotidx/rxgains2gtrelnabypa=0
nvram set pci/2/$slotidx/rxgains5gelnagaina2=3
nvram set pci/2/$slotidx/rxgains5gtrisoa2=6
nvram set pci/2/$slotidx/rxgains5gtrelnabypa=1
nvram set pci/2/$slotidx/maxp5ga2=72,72,72,72
nvram set pci/2/$slotidx/pa5ga2=0xff4d,0x166c,0xfd2a,0xff52,0x168a,0xfd31,0xff5e,0x1768,0xfd25,0xff61,0x1744,0xfd32
nvram set pci/2/$slotidx/cckbw202gpo=0
nvram set pci/2/$slotidx/cckbw20ul2gpo=0
nvram set pci/2/$slotidx/mcsbw202gpo=2571386880
nvram set pci/2/$slotidx/mcsbw402gpo=2571386880
nvram set pci/2/$slotidx/dot11agofdmhrbw202g=17408
nvram set pci/2/$slotidx/ofdmlrbw202gpo=0
nvram set pci/2/$slotidx/mcsbw205glpo=2252472320
nvram set pci/2/$slotidx/mcsbw405glpo=2252472320
nvram set pci/2/$slotidx/mcsbw805glpo=2252472320
nvram set pci/2/$slotidx/mcsbw1605glpo=0
nvram set pci/2/$slotidx/mcsbw205gmpo=2252472320
nvram set pci/2/$slotidx/mcsbw405gmpo=2252472320
nvram set pci/2/$slotidx/mcsbw805gmpo=2252472320
nvram set pci/2/$slotidx/mcsbw1605gmpo=0
nvram set pci/2/$slotidx/mcsbw205ghpo=2252472320
nvram set pci/2/$slotidx/mcsbw405ghpo=2252472320
nvram set pci/2/$slotidx/mcsbw805ghpo=2252472320
nvram set pci/2/$slotidx/mcsbw1605ghpo=0
nvram set pci/2/$slotidx/mcslr5glpo=0
nvram set pci/2/$slotidx/mcslr5gmpo=0
nvram set pci/2/$slotidx/mcslr5ghpo=0
nvram set pci/2/$slotidx/sb20in40hrpo=0
nvram set pci/2/$slotidx/sb20in80and160hr5gl=0
nvram set pci/2/$slotidx/sb40and80hr5glpo=0
nvram set pci/2/$slotidx/sb20in80and160hr5gm=0
nvram set pci/2/$slotidx/sb40and80hr5gmpo=0
nvram set pci/2/$slotidx/sb20in80and160hr5gh=0
nvram set pci/2/$slotidx/sb40and80hr5ghpo=0
nvram set pci/2/$slotidx/sb20in40lrpo=0
nvram set pci/2/$slotidx/sb20in80and160lr5gl=0
nvram set pci/2/$slotidx/sb40and80lr5glpo=0
nvram set pci/2/$slotidx/sb20in80and160lr5gm=0
nvram set pci/2/$slotidx/sb40and80lr5gmpo=0
nvram set pci/2/$slotidx/sb20in80and160lr5gh=0
nvram set pci/2/$slotidx/sb40and80lr5ghpo=0
nvram set pci/2/$slotidx/dot11agduphrpo=0
nvram set pci/2/$slotidx/dot11agduplrpo=0
nvram set pci/2/$slotidx/pcieingress_war=15
nvram set pci/2/$slotidx/sar2g=18
nvram set pci/2/$slotidx/sar5g=15
nvram set pci/2/$slotidx/noiselvl2ga0=31
nvram set pci/2/$slotidx/noiselvl2ga1=31
nvram set pci/2/$slotidx/noiselvl2ga2=31
nvram set pci/2/$slotidx/noiselvl5ga0=31,31,31,31
nvram set pci/2/$slotidx/noiselvl5ga1=31,31,31,31
nvram set pci/2/$slotidx/noiselvl5ga2=31,31,31,31
nvram set pci/2/$slotidx/rxgainerr2ga0=63
nvram set pci/2/$slotidx/rxgainerr2ga1=31
nvram set pci/2/$slotidx/rxgainerr2ga2=31
nvram set pci/2/$slotidx/rxgainerr5ga0=63,63,63,63
nvram set pci/2/$slotidx/rxgainerr5ga1=31,31,31,31
nvram set pci/2/$slotidx/rxgainerr5ga2=31,31,31,31
done

#we only insert wifi modules in init. 
xmldbc -P /etc/services/WIFI/init_wifi_mod.php >> /var/init_wifi_mod.sh
chmod +x /var/init_wifi_mod.sh
/bin/sh /var/init_wifi_mod.sh

#initial wifi interfaces
service PHYINF.WIFI start
