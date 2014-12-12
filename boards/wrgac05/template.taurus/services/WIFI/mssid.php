<?
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";
include "/etc/services/WIFI/function.php";

$UID="BAND24G";

$phy2   = XNODE_getpathbytarget("", "phyinf", "uid", $UID."-1.2", 0);
$phy3   = XNODE_getpathbytarget("", "phyinf", "uid", $UID."-1.3", 0);
$phy4   = XNODE_getpathbytarget("", "phyinf", "uid", $UID."-1.4", 0);
$phy5   = XNODE_getpathbytarget("", "phyinf", "uid", $UID."-1.5", 0);
$mssid1active = query($phy2."/active");
$mssid2active = query($phy3."/active");
$mssid3active = query($phy4."/active");
$mssid4active = query($phy5."/active");

$stsp       = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $UID."-1.1", 0);
$phyp       = XNODE_getpathbytarget("", "phyinf", "uid", $UID."-1.1", 0);
$wifi1  = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
$infp		= XNODE_getpathbytarget("", "inf", "uid", "BRIDGE-1", 0);
$phyinf		= query($infp."/phyinf");
$macaddr	= XNODE_get_var("MACADDR_".$phyinf);//xmldbc -W /runtime/services/globals
$brinf      = query($stsp."/brinf");
$brphyinf   = PHYINF_getphyinf($brinf);
$winfname   = query($stsp."/name");
$beaconinterval     = query($phyp."/media/beacon");
$dtim       = query($phyp."/media/dtim");
$rtsthresh  = query($phyp."/media/rtsthresh");
$fragthresh = query($phyp."/media/fragthresh");
$txpower    = query($phyp."/media/txpower");
$channel = query($phyp."/media/channel");
$shortgi = query($phyp."/media/dot11n/guardinterval");
$bandwidth = query($phyp."/media/dot11n/bandwidth");
$rtsthresh = query($phyp."/media/rtsthresh");
$fragthresh = query($phyp."/media/fragthresh");
$ssid1 = query($wifi1."/ssid");
$opmode = query($wifi1."/opmode");
$ssidhidden = query($wifi1."/ssidhidden");
$wlmode = query($phyp."/media/wlmode");
$wmm = query($phyp."/media/wmm/enable");
$coexist = query($phyp."/media/coexist");
//$iapp = query($phyp."/media/iapp");
$ampdu = query($phyp."/media/ampdu");
$stbc = query($phyp."/media/stbc");
$protection = query($phyp."/media/protection");
$preamble = query($phyp."/media/preamble");
$acl_count  = query($wifi1."/acl/count");
//$acl_max    = query($wifi1."/acl/max");
//$acl_policy     = query($wifi1."/acl/policy");
$fixedrate = query($phyp."/media/txrate");
$mcsindex = query($phyp."/media/dot11n/mcs/index");
$mcsauto = query($phyp."/media/dot11n/mcs/auto");
$txpower = query($phyp."/media/txpower");
$multistream = query($phyp."/media/multistream");

$bssid = 0;
foreach ("/phyinf") {if (query("media/parent") == $UID."-1.1") $bssid++;}
//TRACE_debug("-----------------".$bssid."------------");
//$mssidcount=$bssid-1;
//$mssidcount1=$bssid-1;
//---------------------------follow wlan0 setting----------------------------------------------------//
//while($mssidcount>=0)
//{
//		fwrite("a", $_GLOBALS["START"], 'ifconfig wlan0-va'.$mssidcount.' down\n');
//		fwrite("a", $_GLOBALS["START"], 'brctl delif br0 wlan0-va'.$mssidcount.'\n');
//		$mssidcount=$mssidcount-1;
//}

while($bssid>0)
{
	$mssidcount1=$bssid-1;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib trswitch=1\n');	
//-----------------------------------------set country-------------------------------------------------//	
	$ccode = query("/runtime/devdata/countrycode");
	if($ccode=="US"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib regdomain=1\n');
	}
	else if ($ccode=="JP"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib regdomain=6\n');
	}
	else if ($ccode=="TW"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib regdomain=6\n');
	}
	else if ($ccode=="CN"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib regdomain=3\n');
	}
	else if ($ccode=="GB"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib regdomain=3\n');
	}
	else{
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib regdomain=1\n');
	}
	//----------------------------------set 20/40 bandwidth------------------------------------------------------//	
	if($bandwidth=="20+40"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib use40M=1\n');
		if($channel<5){
			 fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib 2ndchoffset=2\n');
		}
		else{
			fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib 2ndchoffset=1\n');
		}
		if($shortgi==400){
			fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib shortGI40M=1\n');
			fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib shortGI20M=1\n');
		}
		else{
			fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib shortGI40M=0\n');
			fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib shortGI20M=0\n');
		}
	}
	else
	{
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib use40M=0\n');
		if($shortgi==400){
			fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib shortGI40M=0\n');
			fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib shortGI20M=1\n');
			}
		else{
			fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib shortGI40M=0\n');
			fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib shortGI20M=0\n');
		}
	}	
//-------------------------------------set channel------------------------------------------------------------//		
	if ($channel == 0){
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib channel=0\n');
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib disable_ch14_ofdm=1\n');
	}
	else{
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib channel='.$channel.'\n');
	}
//----------------------------------------------------------------------------------------------//
	if($multistream == "2T2R") {fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib MIMO_TR_mode=3\n');}
	else					   {fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib MIMO_TR_mode=4\n');}
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib rtsthres='.$rtsthresh.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib fragthres='.$fragthresh.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib bcnint='.$beaconinterval.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib dtimperiod='.$dtim.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib expired_time=30000\n');
//--------------------------------------------------------------------------------------------------//				
	$temp=$bssid+1;
	setwmm($UID."-1.".$temp,$bssid);
	sethiddenssid($UID."-1.".$temp,$bssid);
	setssid($UID."-1.".$temp,$bssid);
	setband($UID."-1.".$temp,$bssid);
	setfixedrate($UID."-1.".$temp,$bssid);
	guestaccess($UID."-1.".$temp,$bssid);
//-------------------------------------------------------------------------------------------------//				
	if($opmode=="AP"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib opmode=16\n');
	}
	else{
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib opmode=16\n');
	}
//----------------------acl for mssid start------------------------------//
//----------------------acl for mssid end------------------------------//
//-----------------------protection for mssid start-------------------------------//
	if($protection==1){
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib disable_protection=0\n');
	}
	else{
	   fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib disable_protection=1\n');
	}

//-----------------------protection for mssid end-------------------------------//								
	if($preamble=="short"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib preamble=1\n');
	}
	else{
		fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib preamble=0\n');
	}

	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib coexist='.$coexist.'\n');
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib ampdu=1\n');//aggratation.
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib amsdu=1\n');//aggratation.
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib stbc=1\n');				
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0-va'.$mssidcount1.' set_mib wifi_specific=2\n');

	$bssid=$bssid-1;
}
//------------------------------set mssid mac and add interface to bridge------------------------//
$n1 = cut($macaddr, 0, ":");
$n2 = cut($macaddr, 1, ":");
$n3 = cut($macaddr, 2, ":");
$n4 = cut($macaddr, 3, ":");
$n5 = cut($macaddr, 4, ":");
$n6 = cut($macaddr, 5, ":");
if($mssid1active==1){
	$last=$n6+1;
	$mssidmac=$n1.":".$n2.":".$n3.":".$n4.":".$n5.":".$last;
	fwrite("a", $_GLOBALS["START"], 'ip link set wlan0-va0 addr '.$mssidmac.'\n');
	fwrite("a", $_GLOBALS["START"], 'brctl addif br0 wlan0-va0\n');
}
if($mssid2active==1){
	$last=$n6+2;
	$mssidmac=$n1.":".$n2.":".$n3.":".$n4.":".$n5.":".$last;
	fwrite("a", $_GLOBALS["START"], 'ip link set wlan0-va1 addr '.$mssidmac.'\n');
	fwrite("a", $_GLOBALS["START"], 'brctl addif br0 wlan0-va1\n');
}			
if($mssid3active==1){
	$last=$n6+3;
	$mssidmac=$n1.":".$n2.":".$n3.":".$n4.":".$n5.":".$last;
	fwrite("a", $_GLOBALS["START"], 'ip link set wlan0-va2 addr '.$mssidmac.'\n');
	fwrite("a", $_GLOBALS["START"], 'brctl addif br0 wlan0-va2\n');
}		
if($mssid4active==1){
	$last=$n6+4;
	$mssidmac=$n1.":".$n2.":".$n3.":".$n4.":".$n5.":".$last;
	fwrite("a", $_GLOBALS["START"], 'ip link set wlan0-va3 addr '.$mssidmac.'\n');
	fwrite("a", $_GLOBALS["START"], 'brctl addif br0 wlan0-va3\n');
}
?>

