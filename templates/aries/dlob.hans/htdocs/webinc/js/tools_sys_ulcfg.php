<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: null,
	OnLoad: function()
	{
<?
		if ($_GET["RESULT"]=="SUCCESS")
		{
			$bt = query("/runtime/device/bootuptime");
			$delay = 15;
			$bt = $bt + $delay;
			
			/* check file size */
			$filesize = fread("", "/var/session/configsize");
			if($filesize=="" || $filesize=="0")
				echo '\t\tlocation.href="http://'.$_SERVER["HTTP_HOST"].':'.$_SERVER["SERVER_PORT"].'/index.php";';
			else
			{
				unlink("/var/session/configsize");
				
			echo '\t\tvar banner = "'.i18n("Restore Succeeded").'";';
			echo '\t\tvar msgArray = ["'.i18n("The restored configuration file has been uploaded successfully.").'"];';
			echo '\t\tvar sec = '.$bt.';';
			if ($_SERVER["SERVER_PORT"]=="80")
				echo '\t\tvar url = "http://'.$_SERVER["HTTP_HOST"].'/index.php";';
			else
				echo '\t\tvar url = "http://'.$_SERVER["HTTP_HOST"].':'.$_SERVER["SERVER_PORT"].'/index.php";';
			echo 'Service("REBOOT");';
		}
		}
?>	
	},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result) { return true; },
	InitValue: function(xml) { return true; },
	PreSubmit: function() { return null; },
	IsDirty: null,
	Synchronize: function() {}
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
}


function Service(svc)
{	
	var banner = "<?echo i18n("Rebooting");?>...";
	var msgArray = ["<?echo i18n("If you changed the IP address of the AP or wireless stations you will need to change the IP address in your browser before accessing the configuration web page again.");?>"];
	var delay = 10;
	var sec = <?echo query("/runtime/device/bootuptime");?> + delay;
	var url = null;
	var ajaxObj = GetAjaxObj("SERVICE");
	if (svc=="FRESET")		url = "http://192.168.0.50/index.php";
	else if (svc=="REBOOT")	url = "http://<?echo $_SERVER["HTTP_HOST"];?>/index.php";
	else					return false;
	ajaxObj.createRequest();
	ajaxObj.onCallback = function (xml)
	{
		ajaxObj.release();
		if (xml.Get("/report/result")!="OK")
			BODY.ShowAlert("Internal ERROR!\nEVENT "+svc+": "+xml.Get("/report/message"));
		else
			BODY.ShowCountdown(banner, msgArray, sec, url);
	}
	ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
	ajaxObj.sendRequest("service.cgi", "EVENT="+svc);
}


</script>
