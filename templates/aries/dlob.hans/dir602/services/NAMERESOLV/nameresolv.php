<? /* Maker :sam_pan Date: 2010/3/22 03:35 */

include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");} 

fwrite(w,$_GLOBALS["START"], "");
fwrite(w,$_GLOBALS["STOP"], "");
/*setup netbios and llmnr*/
function netbios_setup($name)
{
	$infp = XNODE_getpathbytarget("", "inf", "uid", $name, 0);
	$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $name, 0);
	
	if ($infp=="" || $stsp=="")
	{
		SHELL_info($_GLOBALS["START"], "infsvcs_setup: (".$name.") not exist.");
		SHELL_info($_GLOBALS["STOP"],  "infsvcs_setup: (".$name.") not exist.");
		return;
	}
	$addrtype = query($stsp."/inet/addrtype");
	$ipaddr = query($stsp."/inet/ipv4/ipaddr");
	$devnam = query($stsp."/devnam");
	$hostname = query("device/hostname");
	if ( $devnam == "")
	{
		return;	
	}

	/* add alias hostname */
	/* hostnameWXYZ , WXYZ is the latest 4 digits of MAC address */
	$mac = PHYINF_getmacsetting($name);
	$macstr = cut($mac, 4, ":").cut($mac, 5, ":");
	$ahostname = $hostname.$macstr;

	if($addrtype=="ipv4" )
	{
		$foundip = 0;
		
		$static = query($stsp."/inet/ipv4/static");

		//get dhcp retrieved ip
		if($static=="0") 	{	$ipaddr = query($stsp."/udhcpc/ip/");	}
		if($ipaddr!="")
		{
			$foundip = 1;
			$str_nb = $str_nb."-r ".$hostname.":".$ipaddr." ";
			$str_nb = $str_nb."-r ".$ahostname.":".$ipaddr." ";
		}

		//get autoip and default ip
		foreach($stsp."/ipalias/ipv4/ipaddr")
		{
			/* check if the ipaddr and the alias ip is the same */
			$amask = "";
			$aip = query($stsp."/ipalias/ipv4/ipaddr:".$InDeX);
			$anetid = ipv4networkid($aip, "24");
			if($anetid == "192.168.0.0") $amask = "24";
			if($amask=="")
			{
				$anetid = ipv4networkid($aip, "16");
				if($anetid == "169.254.0.0") $amask = "16";
			}
			$netid = ipv4networkid($ipaddr,$amask);
	
			if($aip !="" && $netid!=$anetid)
			{		
				$foundip = 1;
				$str_nb = $str_nb."-r ".$hostname.":".$aip." ";
				$str_nb = $str_nb."-r ".$ahostname.":".$aip." ";
			}
		}
		
		// always try to kill netbios, if this service is for ipv4
		startcmd("killall netbios");
		
		if($foundip==1)	{	startcmd("netbios -i ".$devnam." ".$str_nb." &\n");	}
		else 			{	startcmd("netbios -i ".$devnam." -r ".$hostname." &\n");	}	
		}	
		
	// always restart llmnresp
	if($addrtype=="ipv4" || $addrtype=="ipv6" )
	{
		startcmd("killall llmnresp");
		startcmd("llmnresp -i ".$devnam." -r ".$hostname." -r ".$ahostname." &\n");
	}
}
?>
