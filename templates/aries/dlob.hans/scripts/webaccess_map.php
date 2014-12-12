#!/bin/sh
<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inet.php";

function unique_partition_name($name)
{
	$check_again = 1;
	while($check_again > 0)
	{
		$check_again = 0;
		foreach("/webaccess/device/entry")
		{
			foreach("entry")
			{
				if ($name==query("uniquename"))
				{
					$check_again = 1;
					$name_suffix = substr($name, strlen($name)-1, 1);
					if (isdigit($name_suffix)=="1")
					{
						$name_suffix = $name_suffix + 1;
						$name = substr($name, 0, strlen($name)-1).$name_suffix;
					}
					else $name = $name."1";	
				}
			}
		}
	}
	return $name;
}

//If the unique partition name is not used by web access settings, the entry contains the device could be deleted first.
$useless_entry = 0;
foreach("/webaccess/device/entry")
{ 
	$useless_entry = $InDeX;
	foreach("entry")
	{
		$uniquename = query("uniquename");
		foreach("/webaccess/account/entry"){ foreach("entry"){ if ($uniquename==cut(query("path"),0 , ":")) $useless_entry = 0;}}
	}
}
if ($useless_entry!=0) del("/webaccess/device/entry:".$useless_entry);

foreach("/webaccess/device/entry")
{
	set("valid", "0");
	foreach("entry"){ set("mntp", "");}
}
foreach("/runtime/device/storage/disk")
{
	//echo "runtime/device/storage \n";
	$disk_n = $InDeX;
	$unique_id = query("serial")."_".query("size");//."_".query("count");
	$is_device_new = "1";
	$cnt = 1;
	foreach("/webaccess/device/entry")
	{
		if ($unique_id==query("uniqueid"))
		{
			del("/webaccess/device/entry:".$cnt);
			$is_device_new = "1";
			break;
/*			set("valid", "1");
			$is_device_new = "0";
			foreach("entry"){ set("mntp", query("/runtime/device/storage/disk:".$disk_n."/entry:".$InDeX."/mntp"));}	
*/			break;
		}
		$cnt = $cnt +1;
	}
	if ($is_device_new=="1")
	{
		$device_n_new = query("/webaccess/device/entry#") + 1;
		add("/webaccess/device/entry:".$device_n_new."/uniqueid", $unique_id);
		add("/webaccess/device/entry:".$device_n_new."/valid", "1");
		foreach("entry")
		{
			$label_name = cut(cut(query("mntp"), 3, "/"), 0, "_");// mntp naming  Vender_Model_SerialLast4Numbers
			if(isempty($label_name) == 1){ $label_name = "Drive";}
			//$unique_partition_name = unique_partition_name($label_name);
			$unique_partition_name = cut(query("mntp"), 3, "/");
			add("/webaccess/device/entry:".$device_n_new."/entry:".$InDeX."/uniquename", $unique_partition_name);
			add("/webaccess/device/entry:".$device_n_new."/entry:".$InDeX."/label", $label_name);
			add("/webaccess/device/entry:".$device_n_new."/entry:".$InDeX."/mntp", query("mntp"));
			add("/webaccess/device/entry:".$device_n_new."/entry:".$InDeX."/space/size", query("space/size"));
			add("/webaccess/device/entry:".$device_n_new."/entry:".$InDeX."/space/used", query("space/used"));
			add("/webaccess/device/entry:".$device_n_new."/entry:".$InDeX."/space/available", query("space/available"));
		}
	}
}
/* Generate /var/run/storage_map file */
$echo_string = "echo -e '";
$idx=0;
foreach("/webaccess/device/entry")
{
	$idx=$idx+1;
	//iphone app enable only one device at same time
	$valid = query("valid");
	if($valid == "1")
	{
		foreach("entry")
		{
			//$echo_string = $echo_string.query("uniquename").":".query("uniquename").":".query("mntp")."\n";
			$echo_string = $echo_string.query("uniquename").":USB_DISK_".$idx.":".query("mntp")."\n";
		}
	}
}
$echo_string = $echo_string."' > /var/run/storage_map";
echo $echo_string;
?>
exit 0
