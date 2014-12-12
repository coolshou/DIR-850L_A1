<? include "/htdocs/webinc/body/draw_elements.php"; ?>
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("EMail Settings");?></h1>
	<p><?echo i18n("The Email feature can be used to send the system log files and router alert messages to your email address.");?></p>
	<p><input type="button" value="<?echo i18n("Save Settings");?>" onClick="BODY.OnSubmit();" />
	    <input type="button" value="<?echo i18n("Don't Save Settings");?>" onClick="BODY.OnReload();" /></p>
</div>
<div class="blackbox">
	<h2><?echo i18n("Email Notification");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable Email Notification");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="en_mail" type="checkbox" onclick="PAGE.OnClickEnable();"/></span>
	</div>
	<br>
</div>
<div class="blackbox">
	<h2><?echo i18n("Email Settings");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("From Email Address");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="from_addr" type="text" size="20" maxlength="128"/></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("To Email Address");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="to_addr" type="text" size="20" maxlength="128"/></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Email Subject");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="email_subject" type="text" size="20" maxlength="128"/></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("SMTP Server Address");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="smtp_server_addr" type="text" size="20" maxlength="128"/></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("SMTP Server Port");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="smtp_server_port" type="text" size="20" maxlength="5"/></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable Authentication");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="authenable" type="checkbox" onclick="PAGE.OnClickAuthEnable();"/></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Account Name");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="account_name" type="text" size="20" maxlength="128"/></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Password");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="passwd" type="password" size="20" maxlength="128"/></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Verify Password");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="verify_passwd" type="password" size="20" maxlength="128"/>
			<input id="sendmail" type="button" value="<?echo i18n("Send Mail Now");?>" onClick="PAGE.OnClickSendMail();"/>
		</span>
	</div>
	<div class="textinput" id="send_msg" style="display:none">
		<span class="name"></span>
		<span class="value"><font color="red"><?echo i18n("(Mail sent already!)");?></font></span>
	</div>	
	<div class="gap"></div>
</div>
<div class="blackbox">
	<h2><?echo i18n("Email log when FULL or on Schedule");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("On Log Full");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="en_logfull" type="checkbox"/></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("On Schedule");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="en_log_sch" type="checkbox" onclick="PAGE.OnClickEnableSchedule();"/></span>
	</div>
<?
if ($FEATURE_NOSCH != "1")
{
	echo '<div class="textinput">';
	echo '<span class="name">'.i18n("Schedule").'</span>';
	echo '<span class="delimiter">:</span>';
	echo '<span class="value">';
	DRAW_select_sch("log_sch", i18n("Never"), "-1", "PAGE.OnChangeSchedule()", 0, "narrow");
	echo '</span></div>';
	echo '<div class="textinput">';
	echo '<span class="name">'.i18n("Detail").'</span>';
	echo '<span class="delimiter">:</span>';
	echo '<span class="value"><input id="log_detail" type="text" size="40"/>';
	echo '</span></div>';
}
?>
	<div class="gap"></div>
</div>
<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
	<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</form>
