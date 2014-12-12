<?
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

function devname($uid)
{
	if($uid=="WIFI-REPEATER") {return "wlan0-vxd";}
	if(strstr($uid,"5G") != "") {$phy_band = "BAND5G"; $wifi_inf = "wlan0";}
	else						{$phy_band = "BAND24G"; $wifi_inf = "wlan1";}
	
	if ($uid==$phy_band."-1.1") 	 {return $wifi_inf;}
	else if ($uid==$phy_band."-1.2") {return $wifi_inf."-va0";}
	else if ($uid==$phy_band."-1.3") {return $wifi_inf."-va1";}
	else if ($uid==$phy_band."-1.4") {return $wifi_inf."-va2";}
	else if ($uid==$phy_band."-1.5") {return $wifi_inf."-va3";}
	else							 {return $wifi_inf."-vxd";}
	return "";
}
function setssid($wifi_uid,$dev)
{
	$phyp = XNODE_getpathbytarget("", "phyinf", "uid", $wifi_uid, 0);
	$wifi1  = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	$ssid = query($wifi1."/ssid");
	$i=0;
	$idx=0;
	$len=strlen($ssid);
	$sub_str=$ssid;
	while($i<$len){
		if( charcodeat($sub_str,$i)=="\\" ||
			charcodeat($sub_str,$i)=="\"" ||
			charcodeat($sub_str,$i)=="$" ||
			charcodeat($sub_str,$i)=="`"){
			$string=$string.substr($sub_str,$idx,$i-$idx);
			$string = $string."\\".charcodeat($sub_str,$i);
			$idx=$i+1;

		}
		$i++;
	}
	if($idx==0){
		$string=$sub_str;
	}
	else if($idx!=$len){
		$string=$string.substr($sub_str,$idx,$len-$idx);
	}	

	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib ssid='.'\"'.$string.'\"'.'\n');

}

function setpassphrase($wifi_uid,$dev)
{
	$phyp 	= XNODE_getpathbytarget("", "phyinf", "uid", $wifi_uid, 0);
	$wifi1  = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	$psk    = query($wifi1."/nwkey/psk/key");

	$i=0;
	$idx=0;
	$len=strlen($psk);
	$sub_str=$psk;
	while($i<$len){
		if( charcodeat($sub_str,$i)=="\\" ||
			charcodeat($sub_str,$i)=="\"" ||
			charcodeat($sub_str,$i)=="$" ||
			charcodeat($sub_str,$i)=="`"){
			$string=$string.substr($sub_str,$idx,$i-$idx);
			$string = $string."\\".charcodeat($sub_str,$i);
			$idx=$i+1;

		}
		$i++;
	}
	if($idx==0){
		$string=$sub_str;
	}
	else if($idx!=$len){
		$string=$string.substr($sub_str,$idx,$len-$idx);
	}

	fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib passphrase='.'\"'.$string.'\"'.'\n');	
}

function setband($wifi_uid,$dev){
	$phyp = XNODE_getpathbytarget("", "phyinf", "uid", $wifi_uid, 0);
	$wifi1  = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	//$wlmode = query($phyp."/realtek/wlmode");
	$wlmode = query($phyp."/media/wlmode");
	if($wlmode=="bgn"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=11\n');
	}
	else if ($wlmode=="b"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=1\n');
	}
	else if ($wlmode=="g"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=3\n');
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib deny_legacy=1\n');
	}
	else if ($wlmode=="n"){
		if(strstr($wifi_uid,"5G") != ""){ //a+n - a
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=12\n');
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib deny_legacy=4\n');
		}
		else{ //b+g+n -b-g
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=11\n');
			fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib deny_legacy=3\n');
		}
	}
	else if ($wlmode=="bg"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=3\n');
	}
	else if ($wlmode=="gn"){ //b+g+n -b
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=11\n');
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib deny_legacy=1\n');
	}
	else if ($wlmode=="a"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=4\n');
	}
	else if ($wlmode=="ac_only"){ //a+n+ac -a-n
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=76\n');
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib deny_legacy=12\n');
	}
	else if ($wlmode=="an"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=12\n');
	}
	else if ($wlmode=="aac"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=68\n');
	}
	else if ($wlmode=="acn"){ //a+n+ac =a
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=76\n');
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib deny_legacy=4\n');
	}
	else if ($wlmode=="ac"){ //a+n+ac
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=76\n');
	}
	else{
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib band=11\n');
	}
}

function setfixedrate($wifi_uid,$dev)
{
	$phyp = XNODE_getpathbytarget("", "phyinf", "uid", $wifi_uid, 0);
	$wifi1  = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	$mcsauto = query($phyp."/media/dot11n/mcs/auto");
	$fixedrate = query($phyp."/media/txrate");
	$mcsindex = query($phyp."/media/dot11n/mcs/index");

	if($mcsauto==1 && $fixedrate=="auto"){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib autorate=1\n');
	}
	else{
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib autorate=0\n');
		if($fixedrate!="auto"){
			if($fixedrate=="1")
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=1\n');
			else if($fixedrate=="2")
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=2\n');
			else if($fixedrate=="5.5")
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=4\n');
			else if($fixedrate=="11")
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=8\n');
			else if($fixedrate=="6")
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=16\n');
			else if($fixedrate=="9")
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=32\n');
			else if($fixedrate=="12")
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=64\n');
			else if($fixedrate=="18")
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=128\n');
			else if($fixedrate=="24")
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=256\n');
			else if($fixedrate=="36")
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=512\n');
			else if($fixedrate=="48")
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=1024\n');
			else//fixrate==54
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=2048\n');
		}
		else{//$mcsauto!=1
			if($mcsindex==0)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=4096\n');
			else if($mcsindex==1)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=8192\n');
			else if($mcsindex==2)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=16384\n');
			else if($mcsindex==3)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=32768\n');
			else if($mcsindex==4)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=65536\n');
			else if($mcsindex==5)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=131072\n');
			else if($mcsindex==6)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=262144\n');
			else if($mcsindex==7)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=524288\n');
			else if($mcsindex==8)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=1048576\n');
			else if($mcsindex==9)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=2097152\n');
			else if($mcsindex==10)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=4194304\n');
			else if($mcsindex==11)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=8388608\n');
			else if($mcsindex==12)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=16777216\n');
			else if($mcsindex==13)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=33554432\n');
			else if($mcsindex==14)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=67108864\n');
			else// $mcsindex==15)
				fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib fixrate=134217728\n');
		}
	}
}

function guestaccess($wifi_uid,$dev)
{
	$phyp = XNODE_getpathbytarget("", "phyinf", "uid", $wifi_uid, 0);
	$wifi1  = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	$guestaccess = query("/acl/obfilter/policy");
	if($guestaccess == "" || $guestaccess == "DISABLE")
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib guest_access=0\n'); //0 -->LAN/WAN
	else
	{
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib guest_access=1\n'); //1-->WAN only
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib block_relay=1\n'); //1-->WAN only
	}
}

function setwmm($wifi_uid,$dev)
{
	$phyp = XNODE_getpathbytarget("", "phyinf", "uid", $wifi_uid, 0);
	$wifi1  = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	$wmm = query($phyp."/media/wmm/enable");
	if($wmm==1){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib qos_enable=1\n');
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib apsd_enable=1\n');
	}
	else{
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib qos_enable=0\n');
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib apsd_enable=0\n');
	}
}

function sethiddenssid($wifi_uid,$dev)
{
	$phyp = XNODE_getpathbytarget("", "phyinf", "uid", $wifi_uid, 0);
	$wifi1  = XNODE_getpathbytarget("/wifi", "entry", "uid", query($phyp."/wifi"), 0);
	$ssidhidden = query($wifi1."/ssidhidden");

	if($ssidhidden==1){
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib hiddenAP=1\n');
	}
	else{
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib hiddenAP=0\n');
	}
}

function setup_txpower($uid)
{
	$phypsts= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $uid, 0);
	$dev=devname($uid);
	if($phypsts!=""){
		$ccka	= query($phypsts."/txpower/ccka");
		$cckb	= query($phypsts."/txpower/cckb");
		$sa		= query($phypsts."/txpower/ht401sa");
		$sb		= query($phypsts."/txpower/ht401sb");
		$sa_5G	= query($phypsts."/txpower/ht401sa_5G");
		$sb_5G	= query($phypsts."/txpower/ht401sb_5G");
	}
	else{return;}
	$phyp	= XNODE_getpathbytarget("", "phyinf", "uid", $uid, 0);
	$txpower= query($phyp."/media/txpower");
	if		($txpower=="70"){$tx_value=3;}
	else if ($txpower=="50"){$tx_value=6;}
	else if ($txpower=="25"){$tx_value=12;}
	else					{$tx_value=17;}

	$max_num_24G_ch=14*2;
	$max_num_5G_ch=196*2;

	if($ccka!="" && $cckb!="" && $sa!="" && $sb!=""){
		$index=0;
		while($index<$max_num_24G_ch){
			$ccka_value=substr($ccka,$index,2);
			$ccka_value=strtoul($ccka_value,16);

			if($ccka_value!=0)
			{
				if($ccka_value-$tx_value>=1){$ccka_value=$ccka_value-$tx_value;}
				else						{$ccka_value=1;}
			}
			$ccka_value=dec2strf("%02x",$ccka_value);
			$pwrlevelCCK_A = $pwrlevelCCK_A.$ccka_value;

			$cckb_value=substr($cckb,$index,2);
			$cckb_value=strtoul($cckb_value,16);
			if($cckb_value!=0)
			{
				if($cckb_value-$tx_value>=1){$cckb_value=$cckb_value-$tx_value;}
				else						{$cckb_value=1;}
			}
			$cckb_value=dec2strf("%02x",$cckb_value);
			$pwrlevelCCK_B = $pwrlevelCCK_B.$cckb_value;

			$sa_value=substr($sa,$index,2);
			$sa_value=strtoul($sa_value,16);
			if($sa_value!=0)
			{
				if($sa_value-$tx_value>=1){$sa_value=$sa_value-$tx_value;}
				else						{$sa_value=1;}
			}
			$sa_value=dec2strf("%02x",$sa_value);
			$pwrlevelHT40_1S_A = $pwrlevelHT40_1S_A.$sa_value;

			$sb_value=substr($sb,$index,2);
			$sb_value=strtoul($sb_value,16);
			if($sb_value!=0)
			{
				if($sb_value-$tx_value>=1){$sb_value=$sb_value-$tx_value;}
				else						{$sb_value=1;}
			}
			$sb_value=dec2strf("%02x",$sb_value);
			$pwrlevelHT40_1S_B = $pwrlevelHT40_1S_B.$sb_value;

			$index=$index+2;
		}
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib pwrlevelCCK_A='.$pwrlevelCCK_A.'\n'); 	
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib pwrlevelCCK_B='.$pwrlevelCCK_B.'\n'); 	
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib pwrlevelHT40_1S_A='.$pwrlevelHT40_1S_A.'\n');
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib pwrlevelHT40_1S_B='.$pwrlevelHT40_1S_B.'\n');
	}
	
	if($sa_5G!="" && $sb_5G!=""){
		$index=0;
		while($index<$max_num_5G_ch){
			$sa_value=substr($sa_5G,$index,2);
			$sa_value=strtoul($sa_value,16);
			if($sa_value!=0)
			{
				if($sa_value-$tx_value>=1){$sa_value=$sa_value-$tx_value;}
				else						{$sa_value=1;}
			}
			$sa_value=dec2strf("%02x",$sa_value);
			$pwrlevel5GHT40_1S_A = $pwrlevel5GHT40_1S_A.$sa_value;

			$sb_value=substr($sb_5G,$index,2);
			$sb_value=strtoul($sb_value,16);
			if($sb_value!=0)
			{
				if($sb_value-$tx_value>=1){$sb_value=$sb_value-$tx_value;}
				else						{$sb_value=1;}
			}
			$sb_value=dec2strf("%02x",$sb_value);
			$pwrlevel5GHT40_1S_B = $pwrlevel5GHT40_1S_B.$sb_value;

			$index=$index+2;
		}
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib pwrlevel5GHT40_1S_A='.$pwrlevel5GHT40_1S_A.'\n');
		fwrite("a", $_GLOBALS["START"], 'iwpriv '.$dev.' set_mib pwrlevel5GHT40_1S_B='.$pwrlevel5GHT40_1S_B.'\n');
	}
}

function get_mssid_mac($host_mac,$offset)
{
	$index = 5;
	$mssid_mac = "";
	$carry = 0;

	//loop from low byte to high byte
	//ex: 00:01:02:03:04:05
	//05 -> 04 -> 03 -> 02 -> 01 -> 00
	while($index >= 0)
	{
		$field = cut($host_mac , $index , ":");

		//check mac format
		if($field == "")
			return "";

		//to value
		$value = strtoul($field , 16);
		if($value == "")
			return "";

		if($index == 5)
			$value = $value + $offset;

		//need carry?
		$value = $value + $carry;
		if($value > 255)
		{
			$carry = 1;
			$value = $value % 256;
		}
		else
			$carry = 0;

		//from dec to hex
		$hex_value = dec2strf("%02X" , $value);

		if($mssid_mac == "")
			$mssid_mac = $hex_value;
		else
			$mssid_mac = $hex_value.":".$mssid_mac;

		$index = $index - 1;
	}

	return $mssid_mac;
}
?>

