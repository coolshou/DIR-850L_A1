HTTP/1.1 200 OK
Content-Type: text/xml

<?echo "<?";?>xml version="1.0" encoding="utf-8"<?echo "?>";?>
<postxml>
<? include "/htdocs/phplib/trace.php";


function is_power_user()
{
	if($_GLOBALS["AUTHORIZED_GROUP"] == "")
	{
		return 0;
	}
	if($_GLOBALS["AUTHORIZED_GROUP"] < 0)
	{
		return 0;
	}
	return 1;
}

if ($_POST["CACHE"] == "true")
{
	echo dump(1, "/runtime/session/".$SESSION_UID."/postxml");
}
else
{
	if(is_power_user() == 1)
	{
		/* cut_count() will return 0 when no or only one token. */
		$SERVICE_COUNT = cut_count($_POST["SERVICES"], ",");
		TRACE_debug("GETCFG: got ".$SERVICE_COUNT." service(s): ".$_POST["SERVICES"]);
		$SERVICE_INDEX = 0;
		while ($SERVICE_INDEX < $SERVICE_COUNT)
		{
			$GETCFG_SVC = cut($_POST["SERVICES"], $SERVICE_INDEX, ",");
			TRACE_debug("GETCFG: serivce[".$SERVICE_INDEX."] = ".$GETCFG_SVC);
			if ($GETCFG_SVC!="")
			{
				$file = "/htdocs/webinc/getcfg/".$GETCFG_SVC.".xml.php";
				/* GETCFG_SVC will be passed to the child process. */
				if (isfile($file)=="1") dophp("load", $file);
			}
			$SERVICE_INDEX++;
		}
	}
	else
	{
		/* not a power user, return error message */
		echo "\t<result>FAILED</result>\n";
		echo "\t<message>Not authorized</message>\n";
	}
}
?></postxml>
