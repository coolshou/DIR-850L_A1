HTTP/1.1 200 OK
Content-Type: text/xml

<?
if ($AUTHORIZED_GROUP < 0)
{
	$result = "Authenication fail";
}
else
{
	if ($_POST["act"] == "dnsquery")
	{
		$param="dnsquery -p -t 2 -d mydlink.com -d dlink.com -d dlink.com.cn -d dlink.com.tw -d google.com -d www.mydlink.com -d www.dlink.com -d www.dlink.com.cn -d www.dlink.com.tw -d www.google.com";
		setattr("/runtime/diagnostic/dnsquery", "get", $param);
		$result = get("s","/runtime/diagnostic/dnsquery");
		del("/runtime/diagnostic/dnsquery");
	}
}
echo '<?xml version="1.0"?>\n';
?><dnsquery>
	<report><?=$result?></report>
</dnsquery>