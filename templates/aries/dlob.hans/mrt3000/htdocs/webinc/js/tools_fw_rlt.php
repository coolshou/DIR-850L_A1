<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: null,
	OnLoad: function()
	{
<?
		include "/htdocs/phplib/trace.php";
		$referer = $_SERVER["HTTP_REFERER"];
		$t = 0;

		if ($_GET["PELOTA_ACTION"]=="fwupdate")
		{
			if ($_GET["RESULT"]=="SUCCESS")
			{
				$size	= fread("j","/var/session/imagesize"); if ($size == "") $size = "4000000";
				$fptime	= query("/runtime/device/fptime");
				$bt		= query("/runtime/device/bootuptime");
				$delay	= 10;
				$t		= $size/64000*$fptime/1000+$bt+$delay+20;
				$title	= I18N("j","Firmware Upload");
				$message= '"'.I18N("j","The device is updating the firmware now.").'", '.
						  '"'.I18N("j","It takes a while to update firmware and reboot the device.").
						  ' '.I18N("j","Please DO NOT power off the device.").'"';
			}
			else
			{
				$title = I18N("j","Firmware Upload Failed");
				$btn = "'<input type=\"button\" value=\"".I18N("j","Continue")."\" onclick=\"self.location=\\'tools_firmware.php\\';\">'";
				if ($_GET["REASON"]=="ERR_NO_FILE")
				{
					$message = "'".I18N("j","No image file.")." ".I18N("j","Please select the correct image file and upload it again.")."', ".$btn;
				}
				else if ($_GET["REASON"]=="ERR_INVALID_SEAMA" || $_GET["REASON"]=="ERR_INVALID_FILE")
				{
					$message = "'".I18N("j","Invalid image file.")." ".I18N("j","Please select the correct image file and upload it again.")."', ".$btn;
				}
				else if ($_GET["REASON"]=="ERR_UNAUTHORIZED_SESSION")
				{
					$message = "'".I18N("j","You are unauthorized or have limited authority.")." ".I18N("j","Please Login first.")."', ".$btn;
				}
				else if ($_GET["REASON"]=="ERR_ANOTHER_FWUP_PROGRESS")
				{
					$message = "'".I18N("j","Another image update process is progressing.")." ".I18N("j","If you still want to update the image, please wait until the other process is done and try it again.")."', ".$btn;
				}
			}
		}
		else if ($_POST["ACTION"]=="langupdate")
		{
			TRACE_debug("ACTION=".$_POST["ACTION"]);
			TRACE_debug("FILE=".$_FILES["sealpac"]);
			TRACE_debug("FILETYPES=".$_FILETYPES["sealpac"]);
			$slp = "/var/sealpac/sealpac.slp";
			$title = I18N("j","Update Language Pack");
			if ($_FILES["sealpac"]=="")
			{
				$title = I18N("j","Language Pack Upload Failed");
				$message = "'".I18N("j","Invalid language pack image.")."', ".
							"'<a href=\"".$referer."\">".I18N("j","Click here to return to the previous page.")."</a>'";
			}
			else if (fcopy($_FILES["sealpac"], $slp)!="1")
			{
				$title = I18N("j","Language Pack Upload Failed");
				$message = "'INTERNAL ERROR: fcopy() return error!'";
			}
			else
			{			
				$langcode = sealpac($slp);
				if ($langcode != "")
				{
					$message = "'".I18N("j","You have installed the language pack ($1) successfully.",$langcode)."', ".
								"'<a href=\"".$referer."\">".I18N("j","Click here to return to the previous page.")."</a>'";
					fwrite(w, "/var/sealpac/langcode", $langcode);
					set("/runtime/device/langcode", $langcode);
					event("SEALPAC.SAVE");
				}
				else
				{
					$title = I18N("j","Language Pack Upload Failed");
					$message = "'".I18N("j","Invalid language pack image.")."', ".
								"'<a href=\"".$referer."\">".I18N("j","Click here to return to the previous page.")."</a>'";
					unlink($slp);
				}
			}
		}
		else if ($_POST["ACTION"]=="langclear")
		{
			$title = I18N("j","Clear Language Pack");
			$message = "'".I18N("j","Clearing the language pack ...")."', ".
						"'<a href=\"".$referer."\">".I18N("j","Click here to return to the previous page.")."</a>'";
			set("/runtime/device/langcode", "en");
			event("SEALPAC.CLEAR");
		}
		else
		{
			TRACE_debug("Unknown action - ACTION=".$_POST["ACTION"]);
			$title = I18N("j","Fail - Unknown action");
			$message = "'<a href=\"./index.php\">".I18N("j","Click here to redirect to the home page now.")."</a>'";
			$referer = "./index.php";
		}

		echo "\t\tvar msgArray = [".$message."];\n";
		if ($t > 0)
			echo "\t\tBODY.ShowCountdown(\"".$title."\", msgArray, ".$t.", \"".$referer."\");\n";
		else
			echo "\t\tBODY.ShowMessage(\"".$title."\", msgArray);\n";
?>	},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result) { return true; },
	InitValue: function(xml) { return true; },
	PreSubmit: function() { return null; },
	IsDirty: null,
	Synchronize: function() {}
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
}
</script>
