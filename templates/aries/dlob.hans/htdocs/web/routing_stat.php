HTTP/1.1 200 OK
Content-Type: text/html

<?
include "/htdocs/phplib/trace.php";

if ($AUTHORIZED_GROUP < 0)
{
	echo "Authenication fail";
}
else
{
	$get_routing_cmd = "";
	if($_POST["version"]=="v4")
	{$get_routing_cmd = "echo ==ROUTEN==;route -n;echo ==END1==;echo ==DEFAULT==;ip route show table default;echo ==END2==;echo ==STATIC==;ip route list table STATIC;echo ==END3==;";}
	else if($_POST["version"]=="v6")
	{$get_routing_cmd = "echo ==ROUTEV6==;ip -6 route show ;echo ==END1==;echo ==STATIC==;ip -6 route show table STATIC;echo ==END2==;";}
	if($get_routing_cmd != "")
	{
		$count=cut_count($get_routing_cmd, ";");
		function execute($cmd)
		{
			setattr("/runtime/command", "get", $cmd ." >> /var/cmd.result"); 		
			get("x", "/runtime/command");	
		}
		$i=0;
		unlink("/var/cmd.result");
		while ($i < $count)
		{
			$str = cut($get_routing_cmd, $i, ";"); 
			execute($str);
			$i++;
		}
		
		echo fread("","/var/cmd.result");
		unlink("/var/cmd.result");
	}
	else echo "Invalid parameter!!";	
}
?>



