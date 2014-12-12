<?
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";
include "/etc/services/WIFI/function.php";

$UID24G	= "BAND24G";
$UID5G	= "BAND5G"; 

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function error($err)	{startcmd("exit ".$err); stopcmd("exit ".$err); return $err;}

/**********************************************************************/

/* what we check ?
1. if host is disabled, then our guest must also be disabled !!
*/
function host_guest_dependency_check($uid)
{
	if($uid == $_GLOBALS["UID24G"]."-1.2") 			$host_uid = $_GLOBALS["UID24G"]."-1.1";
	else if ($uid == $_GLOBALS["UID5G"]."-1.2")		$host_uid = $_GLOBALS["UID5G"]."-1.1";
	else return 1;
	
	$p = XNODE_getpathbytarget("", "phyinf", "uid", $host_uid, 0);
	if (query($p."/active")!=1) return 0;
	else 						return 1;
}

function isguestzone($uid)
{
	$postfix = cut($uid, 1,"-");
	$minor = cut($postfix, 1,".");
	if($minor=="2")	return 1;
	else			return 0;
}

function find_brdev($phyinf)
{
	foreach ("/runtime/phyinf")
	{
		if (query("type")!="eth") continue;
		foreach ("bridge/port") if ($VaLuE==$phyinf) {$find = "yes"; break;}
		if ($find=="yes") return query("name");
	}
	return "";
}

function general_setting($wifi_uid,$prefix)
{
	$stsp		= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $wifi_uid, 0);
	$phyp		= XNODE_getpathbytarget("", "phyinf", "uid", $prefix."-1.1", 0); //primary and second ssid use same setting
	$wifi1		= XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	$infp		= XNODE_getpathbytarget("", "inf", "uid", "BRIDGE-1", 0);
	$phyinf		= query($infp."/phyinf");
//	$macaddr	= XNODE_get_var("MACADDR_".$phyinf);//xmldbc -W /runtime/services/globals
	if($prefix==$_GLOBALS["UID24G"])
	{
		$macaddr	= query("/runtime/devdata/wlanmac");
	}
	else
	{
		$macaddr    = query("/runtime/devdata/wlan5mac");
	}
	$brinf		= query($stsp."/brinf");
	$brphyinf	= PHYINF_getphyinf($brinf);
	$winfname	= query($stsp."/name");
	$beaconinterval	= query($phyp."/media/beacon");
	$dtim		= query($phyp."/media/dtim");
	$rtsthresh	= query($phyp."/media/rtsthresh");
	$fragthresh	= query($phyp."/media/fragthresh");
	$txpower	= query($phyp."/media/txpower");
	$channel	= query($phyp."/media/channel");
	$w_partition    = query($wifi1."/acl/isolation");
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
	// We use the /acl/macctrl to replace wifi mac filter db path.
	/*
	$acl_count	= query($wifi1."/acl/count");
	$acl_max	= query($wifi1."/acl/max");
	$acl_policy	= query($wifi1."/acl/policy");
	*/
	$acl_count	= query("/acl/macctrl/count");
	$acl_max	= query("/acl/macctrl/max");
	$acl_policy	= query("/acl/macctrl/policy");
	
	$fixedrate	= query($phyp."/media/txrate");
	$mcsindex	= query($phyp."/media/dot11n/mcs/index");
	$mcsauto	= query($phyp."/media/dot11n/mcs/auto");
	$multistream = query($phyp."/media/multistream");
	$phy2   = XNODE_getpathbytarget("", "phyinf", "uid", $prefix."-1.2", 0); 
	$phy3   = XNODE_getpathbytarget("", "phyinf", "uid", $prefix."-1.3", 0);
	$phy4   = XNODE_getpathbytarget("", "phyinf", "uid", $prefix."-1.4", 0);
	$phy5   = XNODE_getpathbytarget("", "phyinf", "uid", $prefix."-1.5", 0);
	$mssid1active = query($phy2."/active");
	$mssid2active = query($phy3."/active");
	$mssid3active = query($phy4."/active");
	$mssid4active = query($phy5."/active");


	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib trswitch=0\n');
	$ccode = query("/runtime/devdata/countrycode");

	/* When Band 4 is available in our "WiFI Frequency table",
	   pls choose Channel in band 4 but not CH165 as default.  */
	$band4first = 0;

	if($ccode=="US")		{$REGDOMAIN="1";	$PWRIDX="1";	$band4first=1;}
	else if ($ccode=="NA")	{$REGDOMAIN="1";	$PWRIDX="1";	$band4first=1;}
	else if ($ccode=="GB")	{$REGDOMAIN="3";	$PWRIDX="2";	$band4first=0;}
	else if ($ccode=="EU")	{$REGDOMAIN="3";	$PWRIDX="2";	$band4first=0;}
	else if ($ccode=="ES")	{$REGDOMAIN="4";	$PWRIDX="2";	$band4first=0;}
	else if ($ccode=="FR")	{$REGDOMAIN="5";	$PWRIDX="2";	$band4first=0;}
	else if ($ccode=="JP")	{$REGDOMAIN="6";	$PWRIDX="3";	$band4first=0;}
	else if ($ccode=="IL")	{$REGDOMAIN="7";	$PWRIDX="2";	$band4first=0;}
	else if ($ccode=="TW")	{$REGDOMAIN="11";	$PWRIDX="1";	$band4first=1;}
	else if ($ccode=="RU")	{$REGDOMAIN="12";	$PWRIDX="2";	$band4first=1;}
	else if ($ccode=="CN")	{$REGDOMAIN="13";	$PWRIDX="4";	$band4first=1;}	
	else if ($ccode=="KR")	{$REGDOMAIN="14";	$PWRIDX="5";	$band4first=1;}	
	else if ($ccode=="AU")	{$REGDOMAIN="15";	$PWRIDX="6";	$band4first=1;}	
	else if ($ccode=="CA")	{$REGDOMAIN="16";	$PWRIDX="1";	$band4first=1;}
	else if ($ccode=="SG")	{$REGDOMAIN="17";	$PWRIDX="2";	$band4first=1;}
	else if ($ccode=="LA")	{$REGDOMAIN="18";	$PWRIDX="1";	$band4first=1;}
	else if ($ccode=="BR")	{$REGDOMAIN="19";	$PWRIDX="1";	$band4first=1;}
	else					{$REGDOMAIN="1";	$PWRIDX="1";	$band4first=1;}
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib regdomain='.$REGDOMAIN.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib txpwr_lmt_index='.$PWRIDX.'\n');
	//----------------------------------------txpower setting----------------------------------------//
	if($txpower!="100"){setup_txpower($prefix."-1.1");}
	//-----------------------------------------------------------------------------------------------//
	$USE40M="";
	$SECOFFSET="";
	$SGI40M="";
	$SGI20M="";
	if($prefix==$_GLOBALS["UID24G"])
	{
		if($bandwidth=="20+40"){
			$USE40M="1";
			if($channel<5)		{$SECOFFSET="2";}
			else				{$SECOFFSET="1";}
			if($shortgi==400)	{$SGI40M="1";$SGI20M="1";}
			else				{$SGI40M="0";$SGI20M="0";}
		}else{
			$USE40M="0";
			if($shortgi==400)	{$SGI40M="0";$SGI20M="1";}
			else				{$SGI40M="0";$SGI20M="0";}
		}
	}
	else
	{
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
	}
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib use40M='.$USE40M.'\n');
	if($SECOFFSET!=""){fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib 2ndchoffset='.$SECOFFSET.'\n');}
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib shortGI40M='.$SGI40M.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib shortGI20M='.$SGI20M.'\n');

	if ($channel == 0){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib channel=0\n');
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib disable_ch14_ofdm=1\n');
		if ($band4first==1){
			if ($prefix==$_GLOBALS["UID5G"]){
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib ch_low=149\n');
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib ch_hi=161\n');
			}
		}
	}
	else{
		if ($band4first==1){
			if ($prefix==$_GLOBALS["UID5G"]){
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib ch_low=0\n');
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib ch_hi=0\n');
			}
		}
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

	//primary and second ssid use same setting
	setwmm($prefix."-1.1",$winfname);
	setband($prefix."-1.1",$winfname);
	setfixedrate($prefix."-1.1",$winfname);
	//

	if($opmode=="AP"){fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib opmode=16\n');}
	else{fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib opmode=16\n');}
	if($protection==0){fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib disable_protection=1\n');}
	else{fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib disable_protection=0\n');}
	if($preamble=="short"){fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib preamble=1\n');}
	else{fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib preamble=0\n');}
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib coexist='.$coexist.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib ampdu=1\n');//aggratation.
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib amsdu=1\n');//aggratation.
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib stbc=0\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib apsd_enable=0\n');//Enhance Rx cap. Builder
	
	if($w_partition==1){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib block_relay=1\n');
	}
}

function wifi_power_init($drv,$dev,$wlan_prefix)
{
	/*	setattr($wlan_prefix."/txpower/ccka",		"get",	"scut -p pwrlevelCCK_A: /proc/".$dev."/mib_rf");
		setattr($wlan_prefix."/txpower/cckb",		"get",	"scut -p pwrlevelCCK_B: /proc/".$dev."/mib_rf");
		setattr($wlan_prefix."/txpower/ht401sa",	"get",	"scut -p pwrlevelHT40_1S_A: /proc/".$dev."/mib_rf");
		setattr($wlan_prefix."/txpower/ht401sb",	"get",	"scut -p pwrlevelHT40_1S_B: /proc/".$dev."/mib_rf");
		if($drv == "WIFI_5G")
		{
			setattr($wlan_prefix."/txpower/ht401sa_5G",	"get",	"scut -p pwrlevel5GHT40_1S_A: /proc/".$dev."/mib_rf");
			setattr($wlan_prefix."/txpower/ht401sb_5G",	"get",	"scut -p pwrlevel5GHT40_1S_B: /proc/".$dev."/mib_rf");
		}
	*/
	$temp_path = "/runtime/wifi_temp";
	$dev_u = toupper($dev);
	
	setattr($temp_path."/txpower/ccka",	"get","flash get HW_".$dev_u."_TX_POWER_CCK_A|cut -f2 -d=");
	$value = query($temp_path."/txpower/ccka");
	set($wlan_prefix."/txpower/ccka",$value);
	
	setattr($temp_path."/txpower/cckb",	"get","flash get HW_".$dev_u."_TX_POWER_CCK_B|cut -f2 -d=");
	$value = query($temp_path."/txpower/cckb");
	set($wlan_prefix."/txpower/cckb",$value);
	
	setattr($temp_path."/txpower/ht401sa","get","flash get HW_".$dev_u."_TX_POWER_HT40_1S_A|cut -f2 -d=");
	$value = query($temp_path."/txpower/ht401sa");
	set($wlan_prefix."/txpower/ht401sa",$value);
	
	setattr($temp_path."/txpower/ht401sb","get","flash get HW_".$dev_u."_TX_POWER_HT40_1S_B|cut -f2 -d=");
	$value = query($temp_path."/txpower/ht401sb");
	set($wlan_prefix."/txpower/ht401sb",$value);
	
	if($drv == "WIFI_5G")
	{
		setattr($temp_path."/txpower/ht401sa_5G","get","flash get HW_".$dev_u."_TX_POWER_5G_HT40_1S_A|cut -f2 -d=");
		$value = query($temp_path."/txpower/ht401sa_5G");
		set($wlan_prefix."/txpower/ht401sa_5G",$value);
		
		setattr($temp_path."/txpower/ht401sb_5G","get","flash get HW_".$dev_u."_TX_POWER_5G_HT40_1S_B|cut -f2 -d=");
		$value = query($temp_path."/txpower/ht401sb_5G");
		set($wlan_prefix."/txpower/ht401sb_5G",$value);
	}
	del($temp_path);
}

function wifi_service($wifi_uid,$prefix)
{
	$stsp		= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $wifi_uid, 0);
	$phyp		= XNODE_getpathbytarget("", "phyinf", "uid", $wifi_uid, 0);
	$wifi1		= XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	$infp		= XNODE_getpathbytarget("", "inf", "uid", "BRIDGE-1", 0);
	$phyinf		= query($infp."/phyinf");
	if($prefix==$_GLOBALS["UID24G"])
	{
		$macaddr = query("/runtime/devdata/wlanmac");
	}
	else
	{
		$macaddr = query("/runtime/devdata/wlan5mac");
	}
	$brinf		= query($stsp."/brinf");
	$brphyinf	= PHYINF_getphyinf($brinf);
	$winfname	= query($stsp."/name");
	$phy2   = XNODE_getpathbytarget("", "phyinf", "uid", $prefix."-1.2", 0); 
	$phy3   = XNODE_getpathbytarget("", "phyinf", "uid", $prefix."-1.3", 0);
	$phy4   = XNODE_getpathbytarget("", "phyinf", "uid", $prefix."-1.4", 0);
	$phy5   = XNODE_getpathbytarget("", "phyinf", "uid", $prefix."-1.5", 0);
	$mssid1active = query($phy2."/active");
	$mssid2active = query($phy3."/active");
	$mssid3active = query($phy4."/active");
	$mssid4active = query($phy5."/active");

	fwrite("a", $_GLOBALS["START"], 'ifconfig '.$winfname.' down\n');
	fwrite("a", $_GLOBALS["START"], 'flash set_mib '.$winfname.'\n');
	fwrite("a", $_GLOBALS["START"], 'brctl delif br0 '.$winfname.'\n');
	if ($wifi_uid == $prefix."-1.1")
	{

		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib macPhyMode=2\n');
		if($prefix==$_GLOBALS["UID24G"])
		{
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib phyBandSelect=1\n');
		}
		else
		{
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib phyBandSelect=2\n');
		}
//-----------------------------------MSSID setting start-------------------------------------------------------------------//

		if($mssid1active==1 || $mssid2active==1 || $mssid3active==1 || $mssid4active==1){
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib vap_enable=1\n');
		}
			
//-----------------------------------MSSID setting end---------------------------------------------------------------------//
//----------------------------------ACL setting------------------------------------------------------------------//
		// We use the /acl/macctrl to replace wifi mac filter db path.
		/*
			$acl_count	= query($wifi1."/acl/count");
			$acl_max	= query($wifi1."/acl/max");
			$acl_policy	= query($wifi1."/acl/policy");
		*/
		$acl_count	= query("/acl/macctrl/count");
		$acl_max	= query("/acl/macctrl/max");
		$acl_policy	= query("/acl/macctrl/policy");
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib aclnum=0\n');
		if($acl_policy=="ACCEPT")		{$ACLMODE=2;}
		else if ($acl_policy=="DROP")	{$ACLMODE=1;}
		else							{$ACLMODE=0;}
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib aclmode='.$ACLMODE.'\n');
		// We use the /acl/macctrl to replace wifi mac filter db path.
		//foreach ($wifi1."/acl/entry")
		foreach ("/acl/macctrl/entry")
		{
			if ($InDeX > $acl_count || $InDeX > $acl_max) break;
			$acl_enable = query("enable");
			if ($acl_enable == 1)
			{
				$acl_list = query("mac");
				$a = cut($acl_list, "0", ":");
				$a = $a.cut($acl_list, "1", ":");
				$a = $a.cut($acl_list, "2", ":");
				$a = $a.cut($acl_list, "3", ":");
				$a = $a.cut($acl_list, "4", ":");
				$a = $a.cut($acl_list, "5", ":");
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib acladdr='.$a.'\n');
			}
		}
//----------------------------------ACL setting END------------------------------------------------------------------//
		fwrite("a", $_GLOBALS["STOP"], "event WLAN.DISCONNECTED\n");
	}
	else
	{
		guestaccess($wifi_uid,$winfname);
	}

	general_setting($wifi_uid,$prefix);

	$offset = cut($wifi_uid, 1, ".")-1;
	if($offset == 0)
		$mac = $macaddr;
	else
		$mac = get_mssid_mac($macaddr, $offset); 

	fwrite("a", $_GLOBALS["START"], 'ip link set '.$winfname.' addr '.$mac.'\n');
	fwrite("a", $_GLOBALS["START"], 'brctl addif br0 '.$winfname.'\n');
	fwrite("a", $_GLOBALS["STOP"], 'ifconfig '.$winfname.' down\n');
	fwrite("a", $_GLOBALS["STOP"], 'brctl delif br0 '.$winfname.'\n');
}

function wifi_repeater_service($wifi_uid,$prefix)
{
	$stsp		= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $wifi_uid, 0);
	$phyp		= XNODE_getpathbytarget("", "phyinf", "uid", $wifi_uid, 0);
	$wifi1		= XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	$infp		= XNODE_getpathbytarget("", "inf", "uid", "BRIDGE-1", 0);
	$phyinf		= query($infp."/phyinf");
	$brinf		= query($stsp."/brinf");
	$brphyinf	= PHYINF_getphyinf($brinf);
	$winfname	= query($stsp."/name");

	$authtype = query($wifi1."/authtype");
	$encrtype = query($wifi1."/encrtype");
	$psk    = query($wifi1."/nwkey/psk/key");
	$wep_key_len = query($wifi1."/nwkey/wep/size");
	$wep_defkey = query($wifi1."/nwkey/wep/defkey") - 1;
	$ascii = query($wifi1."/nwkey/wep/ascii");
	$wep_key_1 = query($wifi1."/nwkey/wep/key:1");
	$wep_key_2 = query($wifi1."/nwkey/wep/key:2");
	$wep_key_3 = query($wifi1."/nwkey/wep/key:3");
	$wep_key_4 = query($wifi1."/nwkey/wep/key:4");
	if($ascii==1)
	{
		$wep_key_1 = ascii($wep_key_1);
		$wep_key_2 = ascii($wep_key_2);
		$wep_key_3 = ascii($wep_key_3);
		$wep_key_4 = ascii($wep_key_4);
	}

	fwrite("a", $_GLOBALS["START"], 'ifconfig '.$winfname.' down\n');
	fwrite("a", $_GLOBALS["START"], 'flash set_mib '.$winfname.'\n');
	fwrite("a", $_GLOBALS["START"], 'brctl delif br0 '.$winfname.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-vxd copy_mib\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-vxd set_mib opmode=8\n');

	setssid($wifi_uid,$winfname);
//---------------------------Wireless Security start-------------------------------------------------------//
	if($authtype== "OPEN") {
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib authtype=0\n');//open system
	} else if($authtype== "SHARED"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib authtype=1\n');//shared key
	} else{
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib authtype=2\n');//
	}

	if($encrtype== "NONE") {
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib encmode=0\n');//disabled
	} else if($encrtype== "WEP"){
		if($wep_key_len==64){
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib encmode=1\n');//WEP64
		}else if($wep_key_len==128){
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib encmode=5\n');//WEP128

		}
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib wepkey1='.$wep_key_1.'\n');//WEP KEY 1
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib wepkey2='.$wep_key_2.'\n');//WEP KEY 2
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib wepkey3='.$wep_key_3.'\n');//WEP KEY 3
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib wepkey4='.$wep_key_4.'\n');//WEP KEY 4
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib wepdkeyid='.$wep_defkey.'\n');//Default Key Index
	} else{
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib encmode=2\n');//TKIP or AES
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib 802_1x=1\n');
	}

	if($authtype=="WPAPSK"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib psk_enable=1\n');//WPA-PSK
	} else if($authtype=="WPA2PSK"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib psk_enable=2\n');//WPA2-PSK
	} else if($authtype=="WPA+2PSK"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib psk_enable=3\n');//WPA/WPA2-PSK mixed
	} else{
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib psk_enable=0\n');//no psk
	}

	if($authtype=="WPAPSK" || $authtype=="WPA+2PSK"){
		if($encrtype=="TKIP"){
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib wpa_cipher=2\n');
		} else if($encrtype=="AES"){
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib wpa_cipher=8\n');
		} else{
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib wpa_cipher=10\n');
		}
	}
	if($authtype=="WPA2PSK" || $authtype=="WPA+2PSK"){
		if($encrtype=="TKIP"){
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib wpa2_cipher=2\n');
		} else if($encrtype=="AES"){
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib wpa2_cipher=8\n');
		} else{
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib wpa2_cipher=10\n');
		}
	}

	if($authtype=="WPAPSK" || $authtype=="WPA2PSK" || $authtype=="WPA+2PSK"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib passphrase='.$psk.'\n');
	}

	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$winfname.' set_mib wsc_enable=0\n'); //disable wps
//---------------------------Wireless Security end-------------------------------------------------------//

	fwrite("a", $_GLOBALS["START"], 'brctl addif br0 '.$winfname.'\n');
	fwrite("a", $_GLOBALS["START"], 'ifconfig '.$winfname.' up\n');

	fwrite("a", $_GLOBALS["STOP"], 'ifconfig '.$winfname.' down\n');
	fwrite("a", $_GLOBALS["STOP"], 'brctl delif br0 '.$winfname.'\n');
}

function wificonfig($uid)
{
    fwrite(w,$_GLOBALS["START"], "#!/bin/sh\n");
	fwrite(w,$_GLOBALS["STOP"],  "#!/bin/sh\n");
//	fwrite("a",$_GLOBALS["START"], "killall hostapd > /dev/null 2>&1;\n");
//	fwrite("a",$_GLOBALS["STOP"], "killall hostapd > /dev/null 2>&1;\n");
	
	$dev	= devname($uid);
	$prefix = cut($uid, 0,"-");
	if($prefix==$_GLOBALS["UID24G"]) $drv="WIFI";	
	if($prefix==$_GLOBALS["UID5G"]) $drv="WIFI_5G";	
	if($prefix=="WIFI") 		$drv="WIFI";	

	$p = XNODE_getpathbytarget("", "phyinf", "uid", $uid, 0);
	if ($p=="" || $drv=="" || $dev=="")		return error(9);
	if (query($p."/active")!=1) return error(8);
	$wifi = XNODE_getpathbytarget("/wifi",  "entry",  "uid", query($p."/wifi"), 0);
	$opmode = query($wifi."/opmode");
	$isgzone = isguestzone($uid);

	if(host_guest_dependency_check($uid)==0)	return error(8);

	$wlan1=PHYINF_setup($uid, "wifi", $dev);
	setattr($wlan1."/channel_list", "get", "iwlist ".$dev." channel|grep 'Channel list'|sed 's/[ ]*Channel list :[ ]*//g'|sed 's/[ ]/,/g'");
	if($opmode == "AP")
	{
		startcmd("xmldbc -k \"HOSTAPD_RESTARTAP\"");
		if($isgzone!=1)	{wifi_power_init($drv,$dev,$wlan1);}
		wifi_service($uid,$prefix);
	}
	else if($opmode == "REPEATER")
	{
		wifi_repeater_service($uid,$prefix);
	}

	startcmd("rm -f /var/run/".$uid.".DOWN");
	startcmd("echo 1 > /var/run/".$uid.".UP");
	//startcmd("service ".$drv." restart");

	stopcmd("ip link set ".$dev." down");
	stopcmd("echo 1 > /var/run/".$uid.".DOWN");
	stopcmd("rm -f /var/run/".$uid.".UP");

	stopcmd("phpsh /etc/scripts/delpathbytarget.php BASE=/runtime NODE=phyinf TARGET=uid VALUE=".$uid);

	/* define WFA related info for hostapd */
	$dtype  = "urn:schemas-wifialliance-org:device:WFADevice:1";
	setattr("/runtime/hostapd/mac",  "get", "devdata get -e lanmac");
	setattr("/runtime/hostapd/guid", "get", "genuuid -s \"".$dtype."\" -m \"".query("/runtime/hostapd/mac")."\"");
	startcmd("phpsh /etc/scripts/wpsevents.php ACTION=ADD"); 
	startcmd("phpsh /etc/scripts/wifirnodes.php UID=".$uid);

	if($opmode == "AP")
	{
		/* +++ upwifistats */
		startcmd("xmldbc -P /etc/services/WIFI/updatewifistats.php -V PHY_UID=".$prefix."-1.1 > /var/run/restart_upwifistats.sh");
		startcmd("sh /var/run/restart_upwifistats.sh");
		stopcmd("xmldbc -P /etc/services/WIFI/updatewifistats.php -V PHY_UID=".$prefix."-1.1 > /var/run/restart_upwifistats.sh;");
		stopcmd("sh /var/run/restart_upwifistats.sh");
		/* --- upwifistats */

		/*if enable gzone this action will run 4 time when wifi restart.
		  we pending this action in 5 seconds..................
		  all restart actions in 5 seconds ,we just run 1 time....
		*/
		startcmd("xmldbc -t \"HOSTAPD_RESTARTAP:3:sh /etc/scripts/restartap_hostapd.sh\"");
		stopcmd("killall hostapd > /dev/null 2>&1");
		//stopcmd("service ".$drv." stop");
		stopcmd("phpsh /etc/services/WIFI/interfacereboot.php UID=".$uid."");
	}
	return error(0);
}

?>
