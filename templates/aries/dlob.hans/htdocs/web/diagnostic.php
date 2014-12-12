HTTP/1.1 200 OK
Content-Type: text/xml

<?
function pvt_shell_injection($parameter)
{
	return "\"".escape("s",$parameter)."\"";
}
if ($AUTHORIZED_GROUP < 0)
{
	$result = "Authenication fail";
}
else
{
	if ($_POST["act"] == "ping")
	{
		$target = pvt_shell_injection($_POST["dst"]);
		set("/runtime/diagnostic/ping", $target);
		$result = "OK";
	}
	else if ($_POST["act"] == "ping6")
	{
		$target = pvt_shell_injection($_POST["dst"]);
		set("/runtime/diagnostic/ping6", $target);		
		$result = get("x", "/runtime/diagnostic/ping6");
	}
	else if ($_POST["act"] == "pingreport")
	{
		$result = get("x", "/runtime/diagnostic/ping");
	}
}
echo '<?xml version="1.0"?>\n';
?><diagnostic>
	<report><?=$result?></report>
</diagnostic>
