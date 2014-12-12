<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("Dynamic DNS");?></h1>
	<p>
		<?echo i18n("The Dynamic DNS feature allows you to host a server (Web, FTP, Game Server, etc...) using a domain name that you have purchased (www.whateveryournameis.com) with your dynamically assigned IP address. Most broadband Internet Service Providers assign dynamic (changing) IP addresses. Using a DDNS service provider, your friends can enter your host name to connect to your game server no matter what your IP address is.");?>
	</p>
	<p id="dsc_dlink" style="display:none">
		<a href="http://www.dlinkddns.com/"><?echo i18n("Sign up for D-Link's Free DDNS service at www.DLinkDDNS.com.");?></a>
	</p>
	
	<p id="dsc_dlink_cn" style="display:none">
		<a href="http://dlinkddns.com.cn/"><?echo i18n("Sign up for D-Link's Free DDNS service at dlinkddns.com.cn.");?></a>
	</p>
	
	<p id="dsc_DYNDNS" style="display:none">
		<a href="http://www.dyndns.com/"><?echo i18n("Sign up for D-Link's Free DDNS service at www.dyndns.com.");?></a>
	</p>
	<p id="dsc_oray" style="display:none">
		Oray.cn <?echo i18n("Peanut Dynamic Domain Resolve Service");?>:<br>
		&nbsp; <a href="http://www.oray.cn/Passport/Passport_Register.asp" target=_blank>* <?echo i18n("Peanut Dynamic Domain Resolve Service");?></a><br>
		&nbsp; <a href="http://www.oray.cn/peanuthull/peanuthull_prouser.htm" target=_blank>* <?echo i18n("Update to Professional Service of Dynamic Domain Resolve of Peanut");?></a><br>
		&nbsp; <a href="http://ask.oray.cn/help/" target=_blank>* <?echo i18n("Help About Dynamic Domain Resolve Service of Peanut");?></a>
	</p>
	
	<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</div>
<div class="blackbox">
	<h2><?echo i18n("Dynamic DNS Settings");?></h2>
	<div class="centerline" align="center">
		<div class="textinput">
			<span class="name"><?echo i18n("Enable DDNS");?></span>
			<span class="delimiter">:</span>
		<span class="value"><input type="checkbox" id="en_ddns" onClick="PAGE.OnClickEnDdns();" /></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Server Address");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="server"  onchange="PAGE.OnChangeServer();">
				<?if ($FEATURE_DLINK_COM_CN=="1")echo '<option value="DLINK.COM.CN">dlinkddns.com.cn</option>\n';?>
					<option value="DLINK">dlinkddns.com(Free)</option>
					<option value="DYNDNS.C">DynDns.org(Custom)</option>
					<option value="DYNDNS">DynDns.org(Free)</option>
					<option value="DYNDNS.S">DynDns.org(Static)</option>
				<?if ($FEATURE_ORAY=="1")echo '<option value="ORAY">Oray.cn(Peanut)</option>\n';?>
				</select>
			</span>
		</div>
		<div id="host_div" class="textinput">
			<span class="name"><?echo i18n("Host Name");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="text" id="host" maxlength="60" size="40"></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("User Account");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="text" id="user" maxlength="16" size="40"></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Password");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="password" id="passwd" maxlength="16" size="40"></span>
		</div>
		<div id="test_div" class="textinput">
			<span class="name">&nbsp;</span>
			<span class="delimiter">&nbsp;</span>
			<span class="value"><input type="button" id="DdnsTest" value="<?echo i18n("DDNS Account Testing");?>" onclick="PAGE.OnClickUpdateNow();"></span>
		</div>
		<div id="report_div">
			<div class="gap"></div>
			<p id="report"></p>
		</div>
		<div id="peanut_status_div" class="textinput" style="display:none; height:auto;">
			<span class="name"><?echo i18n("Linkage Status");?></span>
			<span class="delimiter">:</span>
			<span class="value" id="peanut_status"></span>
		</div>
		<div style="clear:both;"></div>
		<div id="peanut_detail_div" style="display:none;">
			<div class="textinput">
				<span class="name"><?echo i18n("Service Level");?></span>
				<span class="delimiter">:</span>
				<span class="value" id="peanut_level"></span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Net Domain");?></span>
				<span class="delimiter">:</span>
				<span class="value"></span>
			</div>

			<?
			// Here, we don't check the data in the file, 
			// and trust the data is well structured as domain names immediately followed with it status.
			if ( isfile("/var/run/all_ddns_domain_name")==1 )
			{
				$dnstext = fread("r", "/var/run/all_ddns_domain_name");
				//echo $dnstext.'<br>';
				$cnt = scut_count($dnstext, "");
				$no = 0;
				$i  = 0;
				while ($i < $cnt)
				{
					$domain_name = scut($dnstext, $i, "");
					$i++;
					$status = scut($dnstext, $i, "");
					$i++;
					$color ="red";
					$symbol =":&nbsp;&nbsp;";
					if($status=="ON") { $color="green"; $symbol =":&nbsp;&nbsp;&nbsp;";}
					echo '<div class="textinput" style="height: 14px; margin-top: 0px;">\n';
					echo '\t<span class="name"></span>\n';
					echo '\t<span class="delimiter"></span>\n';
					echo '\t<span class="value"><span style="color: '.$color.';">'.$status.'</span>'.$symbol.$domain_name.'</span>\n';
					echo '</div>\n';
				}
			}
			?>
		</div>
	</div>
	<div class="gap"></div>
</div>
<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</form>

