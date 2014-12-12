HTTP/1.1 200 OK
Content-Type: text/xml

<?
if ($AUTHORIZED_GROUP < 0)
{
	echo "<firmware>Authenication fail</firmware>";
}
else
{
	if ($_POST["act"] == "checkreport")
	{
		//$result = get("x", "/runtime/firmware/havenewfirmware");
		$state	= get("x", "/runtime/firmware/state");
		$fwinfo	= fread("","/tmp/fwinfo.xml");
		if($state!="")
		{
			echo "<firmware><state>".$state."</state></firmware>";
		}
		else if($fwinfo!="")
		{
			echo $fwinfo;
		}
		else
		{
			echo "<firmware><state>WAIT</state></firmware>";
		}
	}
}
?>
