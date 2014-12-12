HTTP/1.1 200 OK
Content-Type: text/xml

<?
if ($AUTHORIZED_GROUP < 0)
{
	$result = "Authenication fail";
}
else
{
	if ($_POST["act"] == "ping")
	{
		set("/runtime/diagnostic/ping", $_POST["dst"]);
		$result = "OK";
	}
	else if ($_POST["act"] == "ping6")
	{
		set("/runtime/diagnostic/ping6", $_POST["dst"]);
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
