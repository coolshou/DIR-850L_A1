HTTP/1.1 200 OK
Content-Type: text/xml

<?
echo '<?xml version="1.0" encoding="utf-8"?>\n';
if ($AUTHORIZED_GROUP < 0)
{
	echo "<dlnastate>Authenication fail</dlnastate>";
}
else
{
	echo "<dlnastate>\n";
	echo "	<device>\n";
	echo "		<storage>\n";
	echo "			<count>".query("/runtime/device/storage/count")."</count>\n";
	echo "		</storage>\n";
	echo "	</device>\n";
	$i=0;
	foreach ("/runtime/scan_media")
	{
		
		$current_scanned_file 	= query("total_scan_file"); if($current_scanned_file=="") 	$current_scanned_file="0";
		$total_file				= query("total_file");		if($total_file=="")				$total_file="0";
		$done					= query("scan_done");		if($done=="")					$done="0";
		
		echo 	"<total_file>".$total_file."</total_file>\n".
				"<current_scanned>".$current_scanned_file."</current_scanned>\n".
				"<scan_done>".$done."</scan_done>\n";
	}
	echo "</dlnastate>\n";
}
?>
