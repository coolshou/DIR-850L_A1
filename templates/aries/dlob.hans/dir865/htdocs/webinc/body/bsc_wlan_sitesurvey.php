<form id="mainform">

<div id="site_survey">
	<table id="SiteSurveyTable" name="SiteSurveyTable" border="1">
		<tr bgcolor="#66CCFF">
			<th width="200px"><?echo i18n("SSID");?></th>
			<th width="120px"><?echo i18n("BSSID");?></th>
			<th width="60px"><?echo i18n("Channel");?></th>
			<th width="40px"><?echo i18n("Type");?></th>
			<th width="150px"><?echo i18n("Encrypt");?></th>
			<th width="50px"><?echo i18n("Signal");?></th>
			<th width="50px"><?echo i18n("Select");?></th>
		</tr>			
	</table>
	<div class="gap"></div>	
	<div class="centerline">
		<input type="button" value="<?echo i18n("Connect");?>" onClick="PAGE.OnClickConnect();" />&nbsp;&nbsp;
		<input type="button" value="<?echo i18n("Exit");?>" onClick="PAGE.OnClickExit();" />&nbsp;&nbsp;
	</div>
	<div class="gap"></div>	
</div>

</form>
