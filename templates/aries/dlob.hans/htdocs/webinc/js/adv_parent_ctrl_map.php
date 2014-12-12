<style>
/* The CSS is only for this page.
 * Notice:
 *	If the items are few, we put them here,
 *	If the items are a lot, please put them into the file, htdocs/web/css/$TEMP_MYNAME.css.
 */
</style>

<script type="text/javascript">
function Page() {}
Page.prototype =
{
	services: "OPENDNS4.MAP,INET.LAN-1,REBOOT",
	OnLoad: function() {},
	OnUnload: function() {},
	OnSubmitCallback: function (code, result)
	{
		BODY.ShowContent();
		switch (code)
		{
		case "OK":
			if(this.reboot===1)
			{
				var msgArray =
				[
					'<?echo I18N("j","It would spend a little time, please wait");?>...',
					'<div class="centerline"><?echo I18N("j","Waiting time");?> : ',
					'<span id="parent_reboot_timer" style="color:red;"></span></div>'
				];
				BODY.ShowMessage('<?echo i18n("Enable DNS Relay");?>...', msgArray);
				this.RebootCountDown();
			}	
			else this.ShowSuccessConfig();
			break;
		case "BUSY":
			BODY.ShowAlert("<?echo i18n("Someone is configuring the device, please try again later.");?>");
			break;
		case "HEDWIG":
			if (result.Get("/hedwig/result")=="FAILED")	BODY.ShowAlert(result.Get("/hedwig/message"));
			break;
		case "PIGWIDGEON":
			BODY.ShowAlert(result.Get("/pigwidgeon/message"));
			break;
		}
		return true;
	},
	InitValue: function(xml)
	{
		PXML.doc = xml;
		var p = PXML.FindModule("OPENDNS4.MAP");
		this.wan1_infp  = GPBT(p, "inf", "uid", "WAN-1", false);
		if(XG(this.wan1_infp+"/open_dns/nonce") !== "<? echo $_GET["nonce"];?>")
		{
			BODY.ShowAlert("<?echo i18n("The nonce is not identical, you may be attacked. Please login OpenDNS again.");?>");
			self.location.href = "./adv_parent_ctrl.php";
		}
		if(XG(this.wan1_infp+"/open_dns/nonce_uptime") < <? echo query("/runtime/device/uptime")-1800;?>)
		{
			BODY.ShowAlert("<?echo i18n("The nonce is expired, you may be attacked. Please login OpenDNS again.");?>");
			self.location.href = "./adv_parent_ctrl.php";
		}		
		return true;
	},
	PreSubmit: function()
	{
		XS(this.wan1_infp+"/open_dns/deviceid", "<? echo $_GET["deviceid"];?>");
		XS(this.wan1_infp+"/open_dns/parent_dns_srv/dns1", "<? echo $_GET["dnsip1"];?>");
		XS(this.wan1_infp+"/open_dns/parent_dns_srv/dns2", "<? echo $_GET["dnsip2"];?>");
		
		/*Enable DNS Relay when openDNS is used*/
		var lan	= PXML.FindModule("INET.LAN-1");
		if(XG(lan+"/inf/dns4")!=="") //DNS Relay is enable already. 
		{
			PXML.IgnoreModule("INET.LAN-1");
			PXML.IgnoreModule("REBOOT");	
		}
		else
		{					
			XS(lan+"/inf/dns4", "DNS4-1");
			PXML.CheckModule("INET.LAN-1", null, null, "ignore");
			this.reboot=1;
		}

		return PXML.doc;
	},
	IsDirty: function()
	{
		return true;
	},
	wan1_infp: null,
	reboot: null,
	bootuptime: <?
		$bt=query("/runtime/device/bootuptime");
		if ($bt=="")	$bt=30;
		else			$bt=$bt+10;
		echo $bt;
	?>,		
	Synchronize: function() {},
	RebootCountDown: function()
	{
		OBJ("parent_reboot_timer").innerHTML = this.bootuptime;
		this.bootuptime--;
		if (this.bootuptime < 1) this.ShowSuccessConfig();
		else setTimeout('PAGE.RebootCountDown()',1000);
	},
	ShowSuccessConfig: function()
	{
		var msgArray =
		[
			'<?echo i18n("You have successfully configured your router to use OpenDNS Parental Control.");?>',
			'<?echo i18n("Do you want to test the function?");?>',
			'<p><input type="button" value="<?echo i18n('Test');?>" onclick="window.open(\'http://www.opendns.com/device/welcome/?device_id=<? echo $_GET["deviceid"];?>\')" /><input type="button" value="<?echo i18n('Return');?>" onClick="self.location.href=\'adv_parent_ctrl.php\';" /></p>'
		];
		BODY.ShowMessage('<?echo i18n("OpenDNS PARENTAL CONTROLS");?>', msgArray);
	}
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////
}
</script>
