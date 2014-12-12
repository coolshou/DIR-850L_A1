<form id="mainform">
<div class="blackbox" id="site_survey" style="border-color: #FF6F00;">
	<h2 style="background-color: #FF6F00;"><?echo i18n("SITE SURVEY");?></h2>
	<div class="centerline"  align="center">
	<table id="SiteSurveyTable" class="general" name="SiteSurveyTable">
			<tr >
			<th width="150px"><?echo i18n("Wi-Fi Network Name");?></th>
			<th width="106px"><?echo i18n("BSSID");?></th>
			<th width="60px"><?echo i18n("Channel");?></th>
			<th width="40px"><?echo i18n("Type");?></th>
			<th width="120px"><?echo i18n("Encrypt");?></th>
			<th width="44px"><?echo i18n("Signal");?></th>
			<th width="70px"><?echo i18n("Select");?></th>
		</tr>			
	</table>
	<div class="gap"></div>	
	<div class="centerline">
		<input type="button" value="<?echo i18n("Connect");?>" onClick="PAGE.OnClickConnect();" />&nbsp;&nbsp;
		<input type="button" value="<?echo i18n("Cancel");?>" onClick="PAGE.OnClickExit();" />&nbsp;&nbsp;
	</div>
	<div class="gap"></div>	
	</div>
</div>
</form>
