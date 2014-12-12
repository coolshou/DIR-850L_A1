<?
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";
include "/etc/services/WIFI/function.php";

$UID="BAND5G";

function wifi_error($errno)
{
	fwrite("a", $_GLOBALS["START"], "event WLAN.DISCONNECTED\n");
	fwrite("a", $_GLOBALS["STOP"],  "event WLAN.DISCONNECTED\n");
	fwrite("a", $_GLOBALS["START"], "exit ".$errno."\n");
	fwrite("a", $_GLOBALS["STOP"],  "exit ".$errno."\n");
}

function general_setting($wifi_uid, $wifi_path)
{
	$stsp		= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $wifi_uid, 0);
	$phyp		= XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID"]."-1.1", 0); //primary and second ssid use same setting
	$wifi1		= XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	$infp		= XNODE_getpathbytarget("", "inf", "uid", "BRIDGE-1", 0);
	$phyinf		= query($infp."/phyinf");
//	$macaddr	= XNODE_get_var("MACADDR_".$phyinf);//xmldbc -W /runtime/services/globals
	$macaddr	= query("/runtime/devdata/wlan5mac");
	$brinf		= query($stsp."/brinf");
	$brphyinf	= PHYINF_getphyinf($brinf);
	$winfname	= query($stsp."/name");
	$beaconinterval	= query($phyp."/media/beacon");
	$dtim		= query($phyp."/media/dtim");
	$rtsthresh	= query($phyp."/media/rtsthresh");
	$fragthresh	= query($phyp."/media/fragthresh");
	$txpower	= query($phyp."/media/txpower");
	$channel	= query($phyp."/media/channel");
	$w_partition    = query($phyp."/media/w_partition");
	$shortgi	= query($phyp."/media/dot11n/guardinterval");
	$bandwidth	= query($phyp."/media/dot11n/bandwidth");
	$rtsthresh	= query($phyp."/media/rtsthresh");
	$fragthresh	= query($phyp."/media/fragthresh");
	$ssid		= query($wifi1."/ssid");
	$opmode		= query($wifi1."/opmode");					
	$ssidhidden	= query($wifi1."/ssidhidden");
	$wlmode		= query($phyp."/media/wlmode");
	$wmm		= query($phyp."/media/wmm/enable");
	$coexist	= query($phyp."/media/dot11n/bw2040coexist");
	$ampdu		= query($phyp."/media/ampdu");
	$protection	= query($phyp."/media/protection");
	$preamble	= query($phyp."/media/preamble");
	$acl_count	= query($wifi1."/acl/count");
	$acl_max	= query($wifi1."/acl/max");
	$acl_policy	= query($wifi1."/acl/policy");
	$fixedrate	= query($phyp."/media/txrate");
	$mcsindex	= query($phyp."/media/dot11n/mcs/index");
	$mcsauto	= query($phyp."/media/dot11n/mcs/auto");
	$multistream = query($phyp."/media/multistream");
	$phy2   = XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID"]."-1.2", 0); 
	$phy3   = XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID"]."-1.3", 0);
	$phy4   = XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID"]."-1.4", 0);
	$phy5   = XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID"]."-1.5", 0);
	$mssid1active = query($phy2."/active");
	$mssid2active = query($phy3."/active");
	$mssid3active = query($phy4."/active");
	$mssid4active = query($phy5."/active");


	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib trswitch=0\n');
	$ccode = query("/runtime/devdata/countrycode");
	if($ccode=="US")		{$REGDOMAIN="1";}
	else if ($ccode=="JP")	{$REGDOMAIN="6";}
	else if ($ccode=="TW")	{$REGDOMAIN="1";}
	else if ($ccode=="CN")	{$REGDOMAIN="3";}	
	else if ($ccode=="GB")	{$REGDOMAIN="3";}	
	else					{$REGDOMAIN="1";}
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib regdomain='.$REGDOMAIN.'\n');
	//----------------------------------------txpower setting----------------------------------------//
	if($txpower!="100"){setup_txpower($_GLOBALS["UID"]."-1.1");}
	//-----------------------------------------------------------------------------------------------//
	$USE40M="";
	$SECOFFSET="";
	$SGI40M="";
	$SGI20M="";
	if($bandwidth=="20+40+80"){
		$USE40M="2";
		if($shortgi==400)	{$SGI40M="1";$SGI20M="1";}
		else				{$SGI40M="0";$SGI20M="0";}
	}
	else if($bandwidth=="20+40"){
		$USE40M="1";
		if($shortgi==400)	{$SGI40M="1";$SGI20M="1";}
		else				{$SGI40M="0";$SGI20M="0";}
	}else{
		$USE40M="0";
		if($shortgi==400)	{$SGI40M="0";$SGI20M="1";}
		else				{$SGI40M="0";$SGI20M="0";}
	}
	if($channel==36 || $channel==44 || $channel==52 || $channel==60 ||
	   $channel==100 || $channel==108 || $channel==116 || $channel==124 ||
	   $channel==132 || $channel==140 || $channel==149 || $channel==157 ||
	   $channel==165 || $channel==173)
	{
		$SECOFFSET="2";
	}
	else
	{
		$SECOFFSET="1";
	}
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib use40M='.$USE40M.'\n');
	if($SECOFFSET!=""){fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib 2ndchoffset='.$SECOFFSET.'\n');}
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib shortGI40M='.$SGI40M.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib shortGI20M='.$SGI20M.'\n');

	if ($channel == 0){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib channel=0\n');
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib disable_ch14_ofdm=1\n');
	}
	else{
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib channel='.$channel.'\n');
	}
	if($multistream == "2T2R") {fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib MIMO_TR_mode=3\n');}
	else					   {fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib MIMO_TR_mode=4\n');}
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib rtsthres=2346\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib fragthres=2346\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib bcnint='.$beaconinterval.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib dtimperiod='.$dtim.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib expired_time=30000\n');

	setssid($wifi_uid,$winfname);
	sethiddenssid($wifi_uid,$winfname);
	setwmm($wifi_uid,$winfname);

	setband($_GLOBALS["UID"]."-1.1",$winfname); //primary and second ssid use same setting
	setfixedrate($_GLOBALS["UID"]."-1.1",$winfname); //primary and second ssid use same setting

	if($opmode=="AP"){fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib opmode=16\n');}
	else{fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib opmode=16\n');}
	if($protection==1){fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib disable_protection=0\n');}
	else{fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib disable_protection=1\n');}
	if($preamble=="short"){fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib preamble=1\n');}
	else{fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib preamble=0\n');}
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib coexist='.$coexist.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib ampdu=1\n');//aggratation.
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib amsdu=1\n');//aggratation.
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib stbc=0\n');
	
	if($w_partition==1){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib block_relay=1\n');
	}
}
function wifi_service($wifi_uid, $wifi_path)
{
	$stsp		= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $wifi_uid, 0);
	$phyp		= XNODE_getpathbytarget("", "phyinf", "uid", $wifi_uid, 0);
	$wifi1		= XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	$infp		= XNODE_getpathbytarget("", "inf", "uid", "BRIDGE-1", 0);
	$phyinf		= query($infp."/phyinf");
//	$macaddr	= XNODE_get_var("MACADDR_".$phyinf);//xmldbc -W /runtime/services/globals
	$macaddr	= query("/runtime/devdata/wlan5mac");
	$brinf		= query($stsp."/brinf");
	$brphyinf	= PHYINF_getphyinf($brinf);
	$winfname	= query($stsp."/name");
	$phy2   = XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID"]."-1.2", 0); 
	$phy3   = XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID"]."-1.3", 0);
	$phy4   = XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID"]."-1.4", 0);
	$phy5   = XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID"]."-1.5", 0);
	$mssid1active = query($phy2."/active");
	$mssid2active = query($phy3."/active");
	$mssid3active = query($phy4."/active");
	$mssid4active = query($phy5."/active");


	if ($wifi_uid == $_GLOBALS["UID"]."-1.1")
	{
		fwrite("a", $_GLOBALS["START"], 'ifconfig '.$winfname.' down\n');
		fwrite("a", $_GLOBALS["START"], 'flash set_mib '.$winfname.'\n');
		fwrite("a", $_GLOBALS["START"], 'brctl delif br0 '.$winfname.'\n');

		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib macPhyMode=2\n');
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib phyBandSelect=2\n');
//-----------------------------------MSSID setting start-------------------------------------------------------------------//

		if($mssid1active==1 || $mssid2active==1 || $mssid3active==1 || $mssid4active==1){
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib vap_enable=1\n');
		}
			
//-----------------------------------MSSID setting end---------------------------------------------------------------------//
//----------------------------------ACL setting------------------------------------------------------------------//
		$acl_count	= query($wifi1."/acl/count");
		$acl_max	= query($wifi1."/acl/max");
		$acl_policy	= query($wifi1."/acl/policy");
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib aclnum=0\n');
		if($acl_policy=="ACCEPT")		{$ACLMODE=1;}
		else if ($acl_policy=="DROP")	{$ACLMODE=2;}
		else							{$ACLMODE=0;}
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib aclmode='.$ACLMODE.'\n');
		foreach ($wifi1."/acl/entry")
		{
			if ($InDeX > $acl_count || $InDeX > $acl_max) break;
			$acl_list = query("mac");
			$a = cut($acl_list, "0", ":");
			$a = $a.cut($acl_list, "1", ":");
			$a = $a.cut($acl_list, "2", ":");
			$a = $a.cut($acl_list, "3", ":");
			$a = $a.cut($acl_list, "4", ":");
			$a = $a.cut($acl_list, "5", ":");
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib acladdr='.$a.'\n');
		}
//----------------------------------ACL setting END------------------------------------------------------------------//
		fwrite("a", $_GLOBALS["STOP"], "event WLAN.DISCONNECTED\n");
	}

	general_setting($wifi_uid, $wifi_path);

	$offset = cut($wifi_uid, 1, ".")-1;
	$n1 = cut($macaddr, 0, ":");
	$n2 = cut($macaddr, 1, ":");
	$n3 = cut($macaddr, 2, ":");
	$n4 = cut($macaddr, 3, ":");
	$n5 = cut($macaddr, 4, ":");
	$n6 = cut($macaddr, 5, ":")+$offset;

	$mac = $n1.":".$n2.":".$n3.":".$n4.":".$n5.":".$n6; 

	fwrite("a", $_GLOBALS["START"], 'ip link set '.$winfname.' addr '.$mac.'\n');
	fwrite("a", $_GLOBALS["START"], 'brctl addif br0 '.$winfname.'\n');
	fwrite("a", $_GLOBALS["STOP"], 'phpsh /etc/scripts/delpathbytarget.php BASE=/runtime NODE=phyinf TARGET=uid VALUE='.$wifi_uid.'\n');
	fwrite("a", $_GLOBALS["STOP"], 'ifconfig '.$winfname.' down\n');
	fwrite("a", $_GLOBALS["STOP"], 'brctl delif br0 '.$winfname.'\n');
}

function wds_service($wifi_uid)
{
	$stsp           = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $wifi_uid, 0);
	$phyp           = XNODE_getpathbytarget("", "phyinf", "uid", $wifi_uid, 0);
	$wifi2          = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	$winfname       = query($stsp."/name");
	
	$ssid 		= 	query($wifi2."/ssid");
	$authtype 	=	query($wifi2."/authtype");	/*such as NONE,OPEN,WEP,WPA,WPA2.*/
	$encrytype 	= 	query($wifi2."/encrtype");
	$wepmode 	= 	query($wifi2."/nwkey/wep/size");
	$wepkey 	= 	query($wifi2."/nwkey/wep/key");
	//$asciimode 	= 	query($wifi2."/nwkey/wep/ascii");
	$wpakey 	= 	query($wifi2."/nwkey/psk/key");

	/*init script*/
	fwrite("a",$_GLOBALS["START"], "#".$wifi_uid." ".$_GLOBALS["UID"]."-2".'\n');	
	fwrite("a",$_GLOBALS["START"], "ifconfig wlan0-vxd down\n");
	fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd copy_mib\n");
	fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib opmode=8\n");
	fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib ssid=".$ssid."\n");

	/*OPEN*/
	if($encrytype == "NONE")
	{
		if($authtype == "OPEN")
		{
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib encmode=0\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib psk_enable=0\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib 802_1x=0\n");
		}
	}

	/*WEP*/
	if($encrytype == "WEP")
	{
		if($wepmode == "64")
		{
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib encmode=1\n");	
			
			/*we not support open mode,only both or shared*/
			if($authtype == "SHARED")
			{
				fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib authtype=1\n");		
			}		
			if($authtype == "BOTH")
			{
				fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib authtype=2\n");		
			}		
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib 802_1x=0\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wepdkeyid=0\n");
			/*here need to confirm xml save data form ,and this value must be HEX*/		
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wepkey1=".$wepkey."\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wepkey2=1111111111\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wepkey3=1111111111\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wepkey4=1111111111\n");				

		}
		if($wepmode == "128")
		{
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib encmode=5\n");	
			
			if($authtype == "SHARED")
			{
				fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib authtype=1\n");		
			}		
			if($authtype == "BOTH")
			{
				fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib authtype=2\n");		
			}		
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib 802_1x=0\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wepdkeyid=0\n");			
			/*here need to confirm xml save data form ,and this value must be HEX*/		
				
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wepkey1=".$wepkey."\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wepkey2=11111111111111111111111111\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wepkey3=11111111111111111111111111\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wepkey4=11111111111111111111111111\n");				
		
		}
	}
	/*WPA*/
	if($authtype == "WPAPSK")
	{
		if($encrytype == "TKIP" || $encrytype == "TKIP+AES")
		{
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib encmode=2\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib psk_enable=1\n");		
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wpa_cipher=2\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib passphrase=".$wpakey."\n");		
		}
		if($encrytype == "AES")
		{
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib encmode=2\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib psk_enable=1\n");		
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wpa_cipher=8\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib passphrase=".$wpakey."\n");		
		}

	}

	/*WPA2 and WPA/WPA2 mix Mode*/
	if($authtype == "WPA+2PSK" || $authtype == "WPA2PSK")
	{
		/*if TKIP+AES,will dispose as TKIP.*/
		if($encrytype == "TKIP" || $encrytype == "TKIP+AES")
		{
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib encmode=2\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib psk_enable=2\n");		
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wpa2_cipher=2\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib passphrase=".$wpakey."\n");		
		}
		if($encrytype == "AES")
		{
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib encmode=2\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib psk_enable=2\n");		
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib wpa2_cipher=8\n");
			fwrite("a",$_GLOBALS["START"], "iwpriv wlan0-vxd set_mib passphrase=".$wpakey."\n");		
		}

	}

	fwrite("a",$_GLOBALS["START"], "ifconfig wlan0-vxd up\n");
	//add wlan0-vxd to br0 bridge.
	fwrite("a",$_GLOBALS["START"], "brctl addif br0 wlan0-vxd\n");

	/* stop script */
	fwrite("a",$_GLOBALS["STOP"], "ifconfig wlan0-vxd down\n");
	//del wlan0-vxd from br0 bridge.
	fwrite("a",$_GLOBALS["STOP"], "brctl delif br0 wlan0-vxd\n");
}

fwrite("w",$START, "#!/bin/sh\n");
fwrite("w", $STOP, "#!/bin/sh\n");

fwrite("a",$START, "killall hostapd > /dev/null 2>&1; sleep 1\n");
fwrite("a",$STOP, "killall hostapd > /dev/null 2>&1; sleep 1\n");

/* Get the phyinf */
$phy1	= XNODE_getpathbytarget("", "phyinf", "uid", $UID."-1.1", 0);	if ($phy1 == "")	return;
//$phyrp1	= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $UID."-1.1", 0);	if ($phyrp1 == "")	return;
//$wifi1	= XNODE_getpathbytarget("/wifi", "entry", "uid", query($phy1."/wifi"), 0);
/* prepare needed config files */
//$winf1	= query($phyrp1."/name");

/* Is the phyinf active? */
$active = query($phy1."/active");

$phy2   = XNODE_getpathbytarget("", "phyinf", "uid", $_GLOBALS["UID"]."-1.2", 0); 
$mssid1active = query($phy2."/active");
/*Get wds phyinf*/
//$phy2   = XNODE_getpathbytarget("", "phyinf", "uid", $UID."-2", 0);   if ($phy2 == "")	return;
//$wifi2  = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phy2."/wifi"), 0);
//$active_wds = query($phy2."/active");

if ($active==1)	{
	$uid=$UID."-1.1";
	$dev=devname($uid);
	$wlan1=PHYINF_setup($uid, "wifi", $dev);

	setattr($wlan1."/txpower/ccka",			"get",	"scut -p pwrlevelCCK_A: /proc/".$dev."/mib_rf");
	setattr($wlan1."/txpower/cckb",			"get",	"scut -p pwrlevelCCK_B: /proc/".$dev."/mib_rf");
	setattr($wlan1."/txpower/ht401sa",		"get",	"scut -p pwrlevelHT40_1S_A: /proc/".$dev."/mib_rf");
	setattr($wlan1."/txpower/ht401sb",		"get",	"scut -p pwrlevelHT40_1S_B: /proc/".$dev."/mib_rf");
	setattr($wlan1."/txpower/ht401sa_5G",	"get",	"scut -p pwrlevel5GHT40_1S_A: /proc/".$dev."/mib_rf");
	setattr($wlan1."/txpower/ht401sb_5G",	"get",	"scut -p pwrlevel5GHT40_1S_B: /proc/".$dev."/mib_rf");
	fwrite("a",$START, "phpsh /etc/scripts/wifirnodes.php UID=".$uid."\n");

	wifi_service($UID."-1.1", $wifi1);
	if ($mssid1active==1){
		$uid=$UID."-1.2";
		$dev=devname($uid);
		PHYINF_setup($uid, "wifi", $dev);
		wifi_service($UID."-1.2", $wifi1);
	}
	fwrite("a",$START, "phpsh /etc/scripts/wpsevents.php ACTION=ADD\n"); 

	/* define WFA related info for hostapd */
	$dtype  = "urn:schemas-wifialliance-org:device:WFADevice:1";
	setattr("/runtime/hostapd/mac",  "get", "devdata get -e lanmac");
	setattr("/runtime/hostapd/guid", "get", "genuuid -s \"".$dtype."\" -m \"".query("/runtime/hostapd/mac")."\"");

    /*
       Win7 logo patch. Hostapd must be restarted ONLY once..!
    */
    if(query("runtime/hostapd_restartap")=="1")
    {
        fwrite("a",$START, "xmldbc -k \"HOSTAPD_RESTARTAP\"\n");
 	    fwrite("a",$START, "xmldbc -t \"HOSTAPD_RESTARTAP:5:sh /etc/scripts/restartap_hostapd.sh\"\n");
    }
    else
    {
        /*if enable gzone this action will run 4 time when wifi restart.
      we pending this action in 3 seconds..................
      all restart actions in 3 seconds ,we just run 1 time....
	    */
        fwrite("a",$START, "xmldbc -k \"HOSTAPD_RESTARTAP\"\n");
 	    fwrite("a",$START, "xmldbc -t \"HOSTAPD_RESTARTAP:5:sh /etc/scripts/restartap_hostapd.sh\"\n");
    }
//	fwrite("a",$START, "xmldbc -P /etc/services/WIFI/hostapdcfg.php > /var/topology.conf\n");
//	fwrite("a",$START, "hostapd /var/topology.conf &\n");
	
	fwrite("a",$START, "event STATUS.GREEN\n");
	fwrite("a",$START, "event WLAN.CONNECTED\n");

	fwrite("a", $START, "xmldbc -P /etc/services/WIFI/updatewifistats.php -V PHY_UID=".$UID."-1.1 > /var/run/restart_upwifistats.sh\n");
	fwrite("a", $START, "sh /var/run/restart_upwifistats.sh\n");
	/*enable/disable wds function*/
	if($active_wds == 1) {wds_service($UID."-2");}
}
else{
	wifi_error("8"); return;
}
fwrite("a",$START, "exit 0\n");
fwrite("a",$STOP,  "exit 0\n");
?>
