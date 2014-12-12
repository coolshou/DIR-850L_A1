<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("WEBSITE FILTER");?></h1>
	<p>
		<?echo I18N("h", 'The Website Filter option allows you to set up a list of Web sites you would like to allow or deny through your network. To use this feature, you must also select the "Apply Web Filter" checkbox in the Access Control section.');?>
	</p>
	<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</div>
<div class="blackbox">
	<h2><?=$URL_MAX_COUNT?> -- <?echo i18n("WEBSITE FILTERING RULES");?></h2>
	<p><?echo i18n("Configure Website Filter below:");?></p>
	<div>	
		&nbsp;	
		<select id="url_mode">
			<option value="ACCEPT"><?echo i18n("ALLOW computers access to ONLY these sites");?></option>
			<option value="DROP"><?echo i18n("DENY computers access to ONLY these sites");?></option>
		</select>
	</div>
	<div class="gap"></div>
	<div>
		&nbsp;
		<input id="clear_url" type="button" value="<?echo i18n("Clear the list below");?>..." onClick="PAGE.OnClickClearURL();" />
	</div>
	<div class="gap"></div>
	
	<div class="centerline" align="center">
		<table id="" class="general">
			<tr  align="center">
				<td colspan="2"><?echo i18n("Website URL");?>/<?echo i18n("Domain");?></td>
			</tr>			
<?
$INDEX = 1;
while ($INDEX <= $URL_MAX_COUNT)
{	
	echo	"<tr  align='center'>"."\n";
	echo	"	<td><input type=text id=url_".$INDEX." style=\"width:250px\" maxlength=99></td>"."\n";
	$INDEX++;
	echo	"	<td><input type=text id=url_".$INDEX." style=\"width:250px\" maxlength=99></td>"."\n";
	$INDEX++;	
	echo	"</tr>"."\n";
}			
?>			
		</table>
	</div>
	<div class="gap"></div>
</div>
<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</form>
