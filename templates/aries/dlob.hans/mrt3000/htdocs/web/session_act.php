HTTP/1.1 200 OK
Content-Type: text/html

<?
if ($AUTHORIZED_GROUP < 0)
{
	echo "Authenication fail";
}
else
{
	echo fread("", "/proc/net/nf_conntrack");
}	
?>
