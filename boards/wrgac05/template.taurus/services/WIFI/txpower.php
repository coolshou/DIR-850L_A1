<?
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

$UID="BAND24G";
	
$phypsts= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $UID."-1.1", 0);
if($phypsts!=""){
	$ccka	= query($phypsts."/txpower/ccka");
	$cckb	= query($phypsts."/txpower/cckb");
	$sa		= query($phypsts."/txpower/ht401sa");
	$sb		= query($phypsts."/txpower/ht401sb");
		}
$phyp	= XNODE_getpathbytarget("", "phyinf", "uid", $UID."-1.1", 0);
$txpower= query($phyp."/media/txpower");

$ccka1=substr($ccka,0,2);	$ccka1=strtoul($ccka1,16);
$ccka2=substr($ccka,6,2);	$ccka2=strtoul($ccka2,16);
$ccka3=substr($ccka,18,2);	$ccka3=strtoul($ccka3,16);
$cckb1=substr($cckb,0,2);	$cckb1=strtoul($cckb1,16);
$cckb2=substr($cckb,6,2);	$cckb2=strtoul($cckb2,16);
$cckb3=substr($cckb,18,2);	$cckb3=strtoul($cckb3,16);
$sa1=substr($sa,0,2);		$sa1=strtoul($sa1,16);
$sa2=substr($sa,6,2);		$sa2=strtoul($sa2,16);
$sa3=substr($sa,18,2);		$sa3=strtoul($sa3,16);
$sb1=substr($sb,0,2);		$sb1=strtoul($sb1,16);
$sb2=substr($sb,6,2);		$sb2=strtoul($sb2,16);
$sb3=substr($sb,18,2);		$sb3=strtoul($sb3,16);

if($txpower=="70"){
	if($ccka1>3){	$ccka1=$ccka1-3;	$ccka1=dec2strf("%02x",$ccka1);}
	else{			$ccka1=1;			$ccka1=dec2strf("%02x",$ccka1);}
	if($ccka2>3){	$ccka2=$ccka2-3;	$ccka2=dec2strf("%02x",$ccka2);}
	else{			$ccka2=1;			$ccka2=dec2strf("%02x",$ccka2);}
	if($ccka3>3){	$ccka3=$ccka3-3;	$ccka3=dec2strf("%02x",$ccka3);}
	else{			$ccka3=1;			$ccka3=dec2strf("%02x",$ccka3);}
	$pwrlevelCCK_A	= $ccka1.$ccka1.$ccka1.$ccka2.$ccka2.$ccka2.$ccka2.$ccka2.$ccka2.$ccka3.$ccka3.$ccka3.$ccka3.$ccka3;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0 set_mib pwrlevelCCK_A='.$pwrlevelCCK_A.'\n'); 	
	if($sa1>3){		$sa1=$sa1-3;		$sa1=dec2strf("%02x",$sa1);}
	else{			$sa1=1;				$sa1=dec2strf("%02x",$sa1);}
	if($sa2>3){		$sa2=$sa2-3;		$sa2=dec2strf("%02x",$sa2);}
	else{			$sa2=1;				$sa2=dec2strf("%02x",$sa2);}
	if($sa3>3){		$sa3=$sa3-3;		$sa3=dec2strf("%02x",$sa3);}
	else{			$sa3=1;				$sa3=dec2strf("%02x",$sa3);}
	$pwrlevelHT40_1S_A	= $sa1.$sa1.$sa1.$sa2.$sa2.$sa2.$sa2.$sa2.$sa2.$sa3.$sa3.$sa3.$sa3.$sa3;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0 set_mib pwrlevelHT40_1S_A='.$pwrlevelHT40_1S_A.'\n');
	if($sb1>3){		$sb1=$sb1-3;		$sb1=dec2strf("%02x",$sb1);}
	else{			$sb1=1;				$sb1=dec2strf("%02x",$sb1);}
	if($sb2>3){		$sb2=$sb2-3;		$sb2=dec2strf("%02x",$sb2);}
	else{			$sb2=1;				$sb2=dec2strf("%02x",$sb2);}
	if($sb3>3){		$sb3=$sb3-3;		$sb3=dec2strf("%02x",$sb3);}
	else{			$sb3=1;				$sb3=dec2strf("%02x",$sb3);}
	$pwrlevelHT40_1S_B	= $sb1.$sb1.$sb1.$sb2.$sb2.$sb2.$sb2.$sb2.$sb2.$sb3.$sb3.$sb3.$sb3.$sb3;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0 set_mib pwrlevelHT40_1S_B='.$pwrlevelHT40_1S_B.'\n');
}else if($txpower=="50"){
	if($ccka1>6){	$ccka1=$ccka1-6;	$ccka1=dec2strf("%02x",$ccka1);}
	else{			$ccka1=1;			$ccka1=dec2strf("%02x",$ccka1);}
	if($ccka2>6){	$ccka2=$ccka2-6;	$ccka2=dec2strf("%02x",$ccka2);}
	else{			$ccka2=1;			$ccka2=dec2strf("%02x",$ccka2);}
	if($ccka3>6){	$ccka3=$ccka3-6;	$ccka3=dec2strf("%02x",$ccka3);}
	else{			$ccka3=1;			$ccka3=dec2strf("%02x",$ccka3);}
	$pwrlevelCCK_A	= $ccka1.$ccka1.$ccka1.$ccka2.$ccka2.$ccka2.$ccka2.$ccka2.$ccka2.$ccka3.$ccka3.$ccka3.$ccka3.$ccka3;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0 set_mib pwrlevelCCK_A='.$pwrlevelCCK_A.'\n');
	if($sa1>6){		$sa1=$sa1-6;		$sa1=dec2strf("%02x",$sa1);}
	else{			$sa1=1;				$sa1=dec2strf("%02x",$sa1);}
	if($sa2>6){		$sa2=$sa2-6;		$sa2=dec2strf("%02x",$sa2);}
	else{			$sa2=1;				$sa2=dec2strf("%02x",$sa2);}
	if($sa3>6){		$sa3=$sa3-6;		$sa3=dec2strf("%02x",$sa3);}
	else{			$sa3=1;				$sa3=dec2strf("%02x",$sa3);}
	$pwrlevelHT40_1S_A	= $sa1.$sa1.$sa1.$sa2.$sa2.$sa2.$sa2.$sa2.$sa2.$sa3.$sa3.$sa3.$sa3.$sa3;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0 set_mib pwrlevelHT40_1S_A='.$pwrlevelHT40_1S_A.'\n');
	if($sb1>6){		$sb1=$sb1-6;		$sb1=dec2strf("%02x",$sb1);}
	else{			$sb1=1;				$sb1=dec2strf("%02x",$sb1);}
	if($sb2>6){		$sb2=$sb2-6;		$sb2=dec2strf("%02x",$sb2);}
	else{			$sb2=1;				$sb2=dec2strf("%02x",$sb2);}
	if($sb3>6){		$sb3=$sb3-6;		$sb3=dec2strf("%02x",$sb3);}
	else{			$sb3=1;				$sb3=dec2strf("%02x",$sb3);}
	$pwrlevelHT40_1S_B	= $sb1.$sb1.$sb1.$sb2.$sb2.$sb2.$sb2.$sb2.$sb2.$sb3.$sb3.$sb3.$sb3.$sb3;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0 set_mib pwrlevelHT40_1S_B='.$pwrlevelHT40_1S_B.'\n');
}else if($txpower=="25"){
	if($ccka1>9){	$ccka1=$ccka1-9;	$ccka1=dec2strf("%02x",$ccka1);}
	else{			$ccka1=1;			$ccka1=dec2strf("%02x",$ccka1);}
	if($ccka2>9){	$ccka2=$ccka2-9;	$ccka2=dec2strf("%02x",$ccka2);}
	else{			$ccka2=1;			$ccka2=dec2strf("%02x",$ccka2);}
	if($ccka3>9){	$ccka3=$ccka3-9;	$ccka3=dec2strf("%02x",$ccka3);}
	else{			$ccka3=1;			$ccka3=dec2strf("%02x",$ccka3);}
	$pwrlevelCCK_A	= $ccka1.$ccka1.$ccka1.$ccka2.$ccka2.$ccka2.$ccka2.$ccka2.$ccka2.$ccka3.$ccka3.$ccka3.$ccka3.$ccka3;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0 set_mib pwrlevelCCK_A='.$pwrlevelCCK_A.'\n');
	if($sa1>9){		$sa1=$sa1-9;		$sa1=dec2strf("%02x",$sa1);}
	else{			$sa1=1;				$sa1=dec2strf("%02x",$sa1);}
	if($sa2>9){		$sa2=$sa2-9;		$sa2=dec2strf("%02x",$sa2);}
	else{			$sa2=1;				$sa2=dec2strf("%02x",$sa2);}
	if($sa3>9){		$sa3=$sa3-9;		$sa3=dec2strf("%02x",$sa3);}
	else{			$sa3=1;				$sa3=dec2strf("%02x",$sa3);}
	$pwrlevelHT40_1S_A	= $sa1.$sa1.$sa1.$sa2.$sa2.$sa2.$sa2.$sa2.$sa2.$sa3.$sa3.$sa3.$sa3.$sa3;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0 set_mib pwrlevelHT40_1S_A='.$pwrlevelHT40_1S_A.'\n');
	if($sb1>9){		$sb1=$sb1-9;		$sb1=dec2strf("%02x",$sb1);}
	else{			$sb1=1;				$sb1=dec2strf("%02x",$sb1);}
	if($sb2>9){		$sb2=$sb2-9;		$sb2=dec2strf("%02x",$sb2);}
	else{			$sb2=1;				$sb2=dec2strf("%02x",$sb2);}
	if($sb3>9){		$sb3=$sb3-9;		$sb3=dec2strf("%02x",$sb3);}
	else{			$sb3=1;				$sb3=dec2strf("%02x",$sb3);}
	$pwrlevelHT40_1S_B	= $sb1.$sb1.$sb1.$sb2.$sb2.$sb2.$sb2.$sb2.$sb2.$sb3.$sb3.$sb3.$sb3.$sb3;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0 set_mib pwrlevelHT40_1S_B='.$pwrlevelHT40_1S_B.'\n');
}else{//txpower==15%
	if($ccka1>17){	$ccka1=$ccka1-17;	$ccka1=dec2strf("%02x",$ccka1);}
	else{			$ccka1=1;			$ccka1=dec2strf("%02x",$ccka1);}
	if($ccka2>17){	$ccka2=$ccka2-17;	$ccka2=dec2strf("%02x",$ccka2);}
	else{			$ccka2=1;			$ccka2=dec2strf("%02x",$ccka2);}
	if($ccka3>17){	$ccka3=$ccka3-17;	$ccka3=dec2strf("%02x",$ccka3);}
	else{			$ccka3=1;			$ccka3=dec2strf("%02x",$ccka3);}
	$pwrlevelCCK_A	= $ccka1.$ccka1.$ccka1.$ccka2.$ccka2.$ccka2.$ccka2.$ccka2.$ccka2.$ccka3.$ccka3.$ccka3.$ccka3.$ccka3;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0 set_mib pwrlevelCCK_A='.$pwrlevelCCK_A.'\n');
	if($sa1>17){	$sa1=$sa1-17;		$sa1=dec2strf("%02x",$sa1);}
	else{			$sa1=1;				$sa1=dec2strf("%02x",$sa1);}
	if($sa2>17){	$sa2=$sa2-17;		$sa2=dec2strf("%02x",$sa2);}
	else{			$sa2=1;				$sa2=dec2strf("%02x",$sa2);}
	if($sa3>17){	$sa3=$sa3-17;		$sa3=dec2strf("%02x",$sa3);}
	else{			$sa3=1;				$sa3=dec2strf("%02x",$sa3);}
	$pwrlevelHT40_1S_A	= $sa1.$sa1.$sa1.$sa2.$sa2.$sa2.$sa2.$sa2.$sa2.$sa3.$sa3.$sa3.$sa3.$sa3;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0 set_mib pwrlevelHT40_1S_A='.$pwrlevelHT40_1S_A.'\n');
	if($sb1>17){	$sb1=$sb1-17;		$sb1=dec2strf("%02x",$sb1);}
	else{			$sb1=1;				$sb1=dec2strf("%02x",$sb1);}
	if($sb2>17){	$sb2=$sb2-17;		$sb2=dec2strf("%02x",$sb2);}
	else{			$sb2=1;				$sb2=dec2strf("%02x",$sb2);}
	if($sb3>17){	$sb3=$sb3-17;		$sb3=dec2strf("%02x",$sb3);}
	else{			$sb3=1;				$sb3=dec2strf("%02x",$sb3);}
	$pwrlevelHT40_1S_B	= $sb1.$sb1.$sb1.$sb2.$sb2.$sb2.$sb2.$sb2.$sb2.$sb3.$sb3.$sb3.$sb3.$sb3;
	fwrite("a", $_GLOBALS["START"], 'iwpriv wlan0 set_mib pwrlevelHT40_1S_B='.$pwrlevelHT40_1S_B.'\n');
	}

?>
