
<?echo "<?";?>xml version="1.0" encoding="utf-8"<?echo "?>";?>

<? include "/htdocs/phplib/trace.php";
echo "<".$SIGNATURE.">\n<runtime>\n<session>\n<".$SESSION.">\n";

TRACE_debug("GETCFG: serivce = ".$GETCFG_SVC);
if ($GETCFG_SVC!="")
{
	$file = "/htdocs/webinc/getcfg/".$GETCFG_SVC.".xml.php";
	/* GETCFG_SVC will be passed to the child process. */
	if (isfile($file)=="1") dophp("load", $file);
}

echo "</".$SESSION.">\n</session>\n</runtime>\n</".$SIGNATURE.">\n";
?>