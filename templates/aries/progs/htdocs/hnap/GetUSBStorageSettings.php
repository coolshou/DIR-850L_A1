HTTP/1.1 200 OK
Content-Type: text/xml; charset=utf-8

<? 
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/webinc/config.php";
include "/htdocs/phplib/trace.php"; 

$result = "OK";
$enable = get("","/webaccess/enable");
$RemoteHttp = get("","/webaccess/httpenable");
$RemoteHttpPort = get("","/webaccess/httpport");
$RemoteHttps = get("","/webaccess/httpsenable");
$RemoteHttpsPort = get("","/webaccess/httpsport");

if($enable==1) $enable = "true"; else $enable = "false";
if($RemoteHttp==1) $RemoteHttp = "true"; else $RemoteHttp = "false";
if($RemoteHttps==1) $RemoteHttps = "true"; else $RemoteHttps = "false";

function print_StorageUserInfo()
{
	echo "<StorageUserInfoLists>";
	foreach("/webaccess/account/entry")
	{
		$username = get("","username");
		$passwd = get("","passwd");
		$path = get("","entry/path");
		$permission = get("","entry/permission");
		
		if($username != "Admin" && $username != "admin")
		{
			if($permission == "rw") $permission = "true";
			else $permission = "false";
			
			echo "<StorageUser>";
			echo "<UserName>".escape("x",$username)."</UserName>";
			echo "<Password>".escape("x",$passwd)."</Password>";
			echo "<AccessPath>".escape("x",$path)."</AccessPath>";
			echo "<Promission>".escape("x",$permission)."</Promission>";
			echo "</StorageUser>";
		}
	}
	echo "</StorageUserInfoLists>";
}

?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
<soap:Body>
<GetUSBStorageSettingsResponse xmlns="http://purenetworks.com/HNAP1/">
	<GetUSBStorageSettingsResult><?=$result?></GetUSBStorageSettingsResult>
	<Enabled><?=$enable?></Enabled>
	<RemoteHttp><?=$RemoteHttp?></RemoteHttp>
	<RemoteHttpPort><?=$RemoteHttpPort?></RemoteHttpPort>
	<RemoteHttps><?=$RemoteHttps?></RemoteHttps>
	<RemoteHttpsPort><?=$RemoteHttpsPort?></RemoteHttpsPort>
	<?print_StorageUserInfo();?>
</GetUSBStorageSettingsResponse>
</soap:Body>
</soap:Envelope>