<form id="mainform" onsubmit="return false;">
<div id="wiz_start" class="orangebox">
	<h1><?echo i18n("OpenDNS PARENTAL CONTROLS");?></h1>
	<div class="gap"></div>
	<p>
		<?echo i18n("You have success map the router to a free OpenDNS account. Do you want to save the settings in the router?");?>
	</p>
	<div class="gap"></div>
	<div class="gap"></div>
	<div class="gap"></div>
	<div class="gap"></div>	
</div>
<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
    <input type="button" value="<?echo i18n("Don't Save Settings");?>" onClick='self.location.href="adv_parent_ctrl.php";' /></p>
</form>
