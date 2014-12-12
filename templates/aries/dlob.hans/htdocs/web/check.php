HTTP/1.1 200 OK
Content-Type: text/xml

<?echo '<?xml version="1.0" encoding="utf-8"?>';?>

<?
include "/htdocs/phplib/trace.php";

/*
This file is called by ajax request. 
This file is created for checking if directory/file exist or not. 
Current is used by adv_itunes.php. 
*/

//$_POST["act"] 		= "checkfile";
//$_POST["dirname"] 	= "/var/run/aaaa";
//$_POST["filename"] 	= "/var/run/aaaa";

if ($AUTHORIZED_GROUP < 0)
{
	$result = "Authenication fail";
}
else
{
	/* where our hd/usb will be mounted to ? */
	$mount_path = "/tmp/storage/";
	
	if ($_POST["act"] == "checkdir")
	{
		if(isdir($mount_path.$_POST["dirname"])==0)
			$result = "NOTEXIST";
		else 
			$result = "EXIST";
	}
	else if ($mount_path.$_POST["act"] == "checkfile")
	{
		if(isfile($_POST["filename"])==0)
			$result = "NOTEXIST";
		else 
			$result = "EXIST";	
	}
}

echo "<checkreport>\n";
echo "	<result>".$result."</result>\n";
echo "</checkreport>\n";
?>
