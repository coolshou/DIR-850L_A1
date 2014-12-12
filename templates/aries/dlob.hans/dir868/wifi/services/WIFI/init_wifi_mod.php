<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";
include "/htdocs/webinc/config.php";

echo "insmod /lib/modules/emf.ko\n";
echo "insmod /lib/modules/igs.ko\n";
echo "insmod /lib/modules/wl_ap.ko\n";

function startcmd($cmd)	{ echo $cmd."\n" ;}

function nvram_country_setup($wlif_bss_idx, $ccode, $pci_path)
{
	/* No-DFS: No use some special channels.
	 * DFS-enable: if detect special channel, then will not use it auto. 
	 * Our design will use No-DFS most.
	 * For SR and RU, they just only have two options: one is DFS enable or no use 5G band. 
	 */
	$ctry_code = $ccode;
	
	if ($ccode == "US") 
	{
		$regrev = 0;
		//$ctry_code = "Q2";	
	}
	else if ($ccode == "CN") 
		$regrev = 0;
	else if ($ccode == "TW")
		$regrev = 0;
	else if ($ccode == "CA") 
	{
		$regrev = 0;
		//$ctry_code = "Q2";	
	}
	else if ($ccode == "KR")
		$regrev = 1;
	else if ($ccode == "JP")
		$regrev = 1;
	else if ($ccode == "AU")
		$regrev = 0;
	else if ($ccode == "SG")
	{
		/* Singaport two choice:
			1. DFS enable= SG/0  or 
			2. No use 5G band= SG/1
		*/
		$regrev = 0;
	}
	else if ($ccode == "LA")
		$regrev = 0;
	else if ($ccode == "IL")
		$regrev = 0;
	else if ($ccode == "EG")
		$regrev = 0;
	else if ($ccode == "BR")
		$regrev = 0;
	else if ($ccode == "RU")
	{
		/* Russia two choice:
			1. DFS enable= RU/1  or 
			2. No use 5G band= RU/0
		*/
		$regrev = 1;
	}
	else if ($ccode == "GB" || $ccode == "EU")
		$regrev = 0;
	else
		$regrev = 0;

	/*echo "nvram set ".$pci_5g_path."regrev=".$regrev."\n";
	echo "nvram set ".$pci_5g_path."ccode=".$ctry_code."\n";*/
	startcmd("nvram set ".$pci_path."regrev=0");
	startcmd("nvram set ".$pci_path."ccode=0");
	
	startcmd("nvram set wl".$wlif_bss_idx."_country_code=".$ctry_code);
	startcmd("nvram set wl".$wlif_bss_idx."_country_rev=".$regrev);
	/* alpha create nvram parameter: it's value include country code and regulatory revision */
	startcmd("nvram set wl".$wlif_bss_idx."_alpha_country_code=".$ctry_code."/".$regrev);
}
$ccode = query("/runtime/devdata/countrycode");
if (isdigit($ccode)==1)
{
	TRACE_error("\n\nError\nWe do not support digital country,assign as US\n\n");	
	$ccode = "US";
}
if ($ccode == "")
{
	TRACE_error("\n\nError\nempty country,assign as US\n\n");	
	$ccode = "US";
}

//set smp_affinity value for performance (tom, 20121226)
//move 5g driver irq to CPU1
echo "echo 2 > /proc/irq/169/smp_affinity\n";
//move 2.4g driver irq to CPU1
echo "echo 2 > /proc/irq/163/smp_affinity\n";

//in SDK 143, slot number is 1 (tom, 2012)
//$pci_2g_path = "pci/1/0/";
$pci_2g_path = "pci/1/1/";
$pci_5g_path = "pci/2/1/";

//initialize country code for for 5g interface
$wlif_bss_idx = 1;
nvram_country_setup($wlif_bss_idx, $ccode, $pci_5g_path);

//initialize country code for for 2.4g interface
$wlif_bss_idx = 0;
nvram_country_setup($wlif_bss_idx, $ccode, $pci_2g_path);

/*for ap client we need set country to dirver this moment. 
  the site survey will run without wlconfig ifname up
  the driver will using default channel (1~11).
  we set the country here via wl command.
*/
startcmd("wl -i ".$BAND24G_DEVNAME." country ".$ccode);
startcmd("wl -i ".$BAND5G_DEVNAME." country ".$ccode);

?>
