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

$mnt_root        = "/tmp/storage";
$port            = "60299";
$model_name      = query("/runtime/device/modelname");
$model_number	 = 1;
$product_url	 = query("/runtime/device/producturl");
$vendor			 = query("/runtime/device/vendor");    //upnpvendor may not exist  use vendor & add "Corporation"
$mac_addr        = query("/runtime/devdata/lanmac");
$mntpath         = query("/runtime/upnpav/dms/mntpath"); 
$partition_count = query("/runtime/device/storage/disk/count");
$dms_name        = query("/upnpav/dms/name");
$active          = query("/upnpav/dms/active");
$rescan          = query("/upnpav/dms/rescan");
$sharepath       = query("/upnpav/dms/sharepath");
$Genericname     = query("/runtime/device/upnpmodelname");
$first_partition = query("/runtime/device/storage/disk/entry/mntp");
$db_path         = $mnt_root;

$descript        = query("/runtime/device/description");

if($Genericname == ""){ $Genericname = $model_name; }

$upnp_uid 	= "UPNP.LAN-1";
$upnpp		= XNODE_getpathbytarget("/runtime/services/http", "server", "uid", $upnp_uid, 0);
$net_inf	= query($upnpp."/ifname");
$dev_url	= query($upnpp."/ipaddr");

//if($descript == ""){ $descript = "D-Link Router"; }
/*
if($sharepath == "" || $sharepath == "/")
{ 
	$sharepath = $mnt_root."/";
	$mntpath   = $mnt_root."/";
	$sharename = $mnt_root;
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
	$sharename = $mnt_root."/".$mntpath;	
}
*/
if($sharepath == "" || $sharepath == "/")  //root
{ 
	$is_root=1;
	$sharepath = $mnt_root."/";
	if($first_partition == "")
		$db_pat = $sharepath;
	else
		$db_path = $first_partition;
}
else
{
	$is_root=0;
	$index = strstr($sharepath, "/");
	if($index == "")
		$db_path = $mnt_root."/".$sharepath."/";
	else
	{	
		$diskname = substr($sharepath, 0, $index);
		$db_path = $mnt_root."/".$diskname."/";
	}
	$sharepath = $mnt_root."/".$sharepath;
}
	
if($partition_count!="" && $partition_count!="0") 
{
	$sd_status = "active";
}
else 
{
	$sd_status = "inactive";
}

$MINIDLNA_CONF = "/var/etc/minidlna.conf";
//$PRESCAN     = "/var/run/prescan_start.sh";

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
		fwrite("a", $START, "echo \"DLNA is disabled !\" > /dev/console\n");		
	}
	else
	{												
		fwrite("a", $START, "echo \"Start minidlna ...\" > /dev/console\n");		
//		fwrite("a", $START, "mkdir -p ".$mntpath.".systemfile/.upnpav-db/\n");
//		fwrite("a", $START, "ScanMedia &\n");
		//+++ siyou, modelNumber must be number only, otherwise the xbox360 won't list our DMS.
//		fwrite("a", $START, "upnp 0 \"".$vendor." Corporation\" \"".$product_url."\" \"Network File Server with USB2.0 and wifi interface\" ". "\"".$Genericname."\" \"".$product_url."\" ".$model_number." ".$mac_addr." ".$dms_name." &\n");
/*				
		fwrite("a", $START, "inotify_uPNP -mqr \"".$sharepath."\" & \n");
		fwrite("a", $START, "sh ".$PRESCAN."\n");
		fwrite("a", $START, $tmp."\n");
*/																
//		fwrite("w", $UPNPAV_CONF, "sharepath=".$sharepath."\n");
//  	fwrite("a", $UPNPAV_CONF, "mntpath=".$mntpath."\n");
//		fwrite("a", $UPNPAV_CONF, "sd_status=".$sd_status."\n");
//		fwrite("a", $UPNPAV_CONF, "sharename=".$sharename."\n");

		$db_path = $db_path."/.system_data";
		$db_file = $db_path."/files.db";	
		//write conf
		fwrite("w", $MINIDLNA_CONF, "port=".$port."\n");
		fwrite("a", $MINIDLNA_CONF, "network_interface=".$net_inf."\n");
		fwrite("a", $MINIDLNA_CONF, "media_dir=".$sharepath."\n");
		fwrite("a", $MINIDLNA_CONF, "friendly_name=".$dms_name."\n");
		fwrite("a", $MINIDLNA_CONF, "db_dir=".$db_path."\n");
		fwrite("a", $MINIDLNA_CONF, "album_art_names=Cover.jpg/cover.jpg/AlbumArtSmall.jpg/albumartsmall.jpg/AlbumArt.jpg/albumart.jpg/Album.jpg/album.jpg/Folder.jpg/folder.jpg/Thumb.jpg/thumb.jpg \n");
		fwrite("a", $MINIDLNA_CONF, "inotify=yes\n");
		fwrite("a", $MINIDLNA_CONF, "presentation_url=http://".$dev_url."/\n");
		fwrite("a", $MINIDLNA_CONF, "notify_interval=900\n");
		fwrite("a", $MINIDLNA_CONF, "serial=none\n");
		fwrite("a", $MINIDLNA_CONF, "model_number=".$model_number."\n");
		//fwrite("a", $MINIDLNA_CONF, "model_name=".$Genericname."\n");
		fwrite("a", $MINIDLNA_CONF, "model_name=Windows Media Connect compatible(".$model_name.")\n"); //jef add for xbox360
		fwrite("a", $MINIDLNA_CONF, "model_url=".$product_url."\n");
		fwrite("a", $MINIDLNA_CONF, "model_descript=".$descript."\n");
		fwrite("a", $MINIDLNA_CONF, "vendor_name=".$vendor." Corporation\n");
		fwrite("a", $MINIDLNA_CONF, "vendor_url=".$product_url."\n");
		
		// jef+ keep db alive for query speed //fwrite("a", $START, "rm -rf ".$db_path." \n");			//+++ Jerry Kao, removed db file before service start.										
	//	if($is_root==0){//anny+ DLNA server only export specific partition when there are more than one partition in the usb storage. Remove original db node and recreate it.
			fwrite("a", $START, "rm -rf ".$db_path." \n");
	//	}
		fwrite("a", $START, "minidlna -f ".$MINIDLNA_CONF." -M & \n");//-M: add memory use limit
		fwrite("a", $START, "sleep 1\n");
		fwrite("a", $START, "minidlna_pid=`ps | grep minidlna | scut -f 1`\n");
		fwrite("a", $START, "if [ \"$minidlna_pid\" == \"\" ] ; then \n");
		fwrite("a", $START, "minidlna -f ".$MINIDLNA_CONF." -M & \n");
		fwrite("a", $START, "fi \n");
		fwrite("a", $START, "sleep 1\n");
		
		fwrite("a", $START, "if [ ! -f ".$db_file." ]; then\n");
		fwrite("a", $START, "killall -9 minidlna\n");
		fwrite("a", $START, "service UPNPAV restart\n");
		fwrite("a", $START, "fi \n");	
		//fwrite("a", $START, "minidlna -f ".$MINIDLNA_CONF." & \n");
		//fwrite("a", $START, "minidlna -d -f ".$MINIDLNA_CONF." \n");	// -d: for DEBUG mode.		
	}
}

if ($active!="1")
{
	fwrite("a", $STOP, "echo \"DLNA is disabled !\" > /dev/console\n");
}
else
{	
	fwrite("a", $STOP, "echo \"Stop minidlna ...\" > /dev/console\n");
	fwrite("a", $STOP, "killall -9 minidlna\n");
	fwrite("a", $STOP, "while [ -n \"${`ps | grep \\\\bminidlna`}\" ]\n");	//+++ Jerry Kao, waiting minidlna stop. //jef modify 20130207
	fwrite("a", $STOP, "do\n");
	fwrite("a", $STOP, "sleep 1;\n");
	fwrite("a", $STOP, "killall -9 minidlna;\n");
	fwrite("a", $STOP, "done\n");	
	
	fwrite("a", $STOP, "rm -rf /var/etc/minidlna.conf \n");
}


?>
