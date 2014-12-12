<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/xnode.php";
/* 
	note: 
	1. $sharepath = $mntpath + $folderpath, default share from "/";
	2. $mnt_root reference to ELBOX_PROGS_PRIV_USBMOUNT_MNT_ROOT in elbox_config.h
	3. $sharepath and $mntpath end word, must be "/", this is very important.
*/

$mnt_root        = "/mnt/HD_a2";
$model_name      = query("/runtime/device/modelname");
$model_number	 = 1;
$product_url	 = query("/runtime/device/producturl");
$vendor			 = query("/runtime/device/vendor");
$mac_addr        = query("/runtime/devdata/lanmac");
$mntpath         = query("/runtime/upnpav/dms/mntpath"); 
$partition_count = query("/runtime/device/storage/disk/count");
$dms_name        = query("/upnpav/dms/name");
$active          = query("/upnpav/dms/active");
$rescan          = query("/upnpav/dms/rescan");
$sharepath       = query("/upnpav/dms/sharepath");

$Genericname = query("/runtime/device/upnpmodelname");
if($Genericname == ""){ $Genericname = $model; }

if($sharepath == "" || $sharepath == "/")
{ 
	$sharepath = $mnt_root."/";
	$mntpath   = $mnt_root."/";
	$sharename = "HD_a2";
}
else	
{			
	$index = strstr($sharepath, "/");
	if($index != "")
	{	
		$diskname = substr($sharepath, 0, $index);
		$mntpath = $mnt_root."/".$diskname."/";
	}
	else
	{
		$mntpath = $mnt_root."/".$sharepath."/";
	}	
	$sharename = "HD_a2/".$sharepath;
	$sharepath = $mnt_root."/".$sharepath."/";			
}
	
if($partition_count!="" && $partition_count!="0") 
{
	$sd_status = "active";
}
else 
{
	$sd_status = "inactive";
}

$UPNPAV_CONF = "/etc/upnpav.conf";
$PRESCAN     = "/var/run/prescan_start.sh";

/*---------------------------------------------------------------------*/
fwrite("w", $START, "#!/bin/sh\n");
fwrite("w", $STOP,  "#!/bin/sh\n");

/* if path not exist, use root path */
if (isdir($sharepath)==0)
{
	$sharepath = $mnt_root."/";
	$mntpath   = $mnt_root."/";	
    fwrite("a", $START, "xmldbc -s /upnpav/dms/sharepath \"/\"\n");
}

if($sd_status == "inactive")
{
	fwrite("a", $START, "echo \"No HD found\"  > /dev/console\n");
}
else
{
	if ($active!="1")
	{
		fwrite("a", $START, "echo \"upnp-av is disabled !\" > /dev/console\n");		
	}
	else
	{														
		fwrite("a", $START, "echo \"Start upnp-av ...\" > /dev/console\n");		
		fwrite("a", $START, "mkdir -p ".$mntpath.".systemfile/.upnpav-db/\n");
		fwrite("a", $START, "ScanMedia &\n");
		//+++ siyou, modelNumber must be number only, otherwise the xbox360 won't list our DMS.
		fwrite("a", $START, "upnp 0 \"".$vendor." Corporation\" \"".$product_url."\" \"Network File Server with USB2.0 and wifi interface\" ". "\"".$Genericname."\" \"".$product_url."\" ".$model_number." ".$mac_addr." ".$dms_name." &\n");
				
		fwrite("a", $START, "inotify_uPNP -mqr \"".$sharepath."\" & \n");
		fwrite("a", $START, "sh ".$PRESCAN."\n");
		fwrite("a", $START, $tmp."\n");
																
		fwrite("w", $UPNPAV_CONF, "sharepath=".$sharepath."\n");
		fwrite("a", $UPNPAV_CONF, "mntpath=".$mntpath."\n");
		fwrite("a", $UPNPAV_CONF, "sd_status=".$sd_status."\n");
		fwrite("a", $UPNPAV_CONF, "sharename=".$sharename."\n");
	}
}

if ($active!="1")
{
	fwrite("a", $STOP, "echo \"upnp-av is disabled !\" > /dev/console\n");
}
else
{	
	fwrite("a", $STOP, "echo \"Stop upnp-av ...\" > /dev/console\n");	
	fwrite("a", $STOP, "killall -9 ScanMedia \n");			
	fwrite("a", $STOP, "killall -9 upnp\n"); 				
	fwrite("a", $STOP, "killall -9 inotify_uPNP\n");	
	fwrite("a", $STOP, "rm -rf /tmp/prescan* \n");
}

/*---------------------------------------------------------------------------*/
fwrite("w", $PRESCAN, "#!/bin/sh\n");

if ($active!="1")
{
	fwrite("a", $PRESCAN, "echo \"Upnp-av server is disabled !\" > /dev/console\n");
}
else
{		
	if($rescan !="0" && $rescan != "")
	{
		fwrite("a", $PRESCAN, "while [ 1=1 ]; do\n");
		fwrite("a", $PRESCAN, "\tsleep ".$rescan."\n");		
		fwrite("a", $PRESCAN, "\tScanMedia \n");	
		fwrite("a", $PRESCAN, "done \n");
	}			
}

?>
