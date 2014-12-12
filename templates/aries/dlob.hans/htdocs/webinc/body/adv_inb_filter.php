<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo I18N("h","INBOUND FILTER");?></h1>
	<p><?echo I18N("h","The Inbound Filter option is an advanced method of controlling data received from the Internet. With this feature you can configure inbound data filtering rules that control data based on an IP address range.");?></p>
	<p><?echo I18N("h","Inbound Filters can be used for limiting access to a server on your network to a system or group of systems. Filter rules can be used with Virtual Server, Port Forwarding, or Remote Administration features.");?></p>	
</div>
<div class="blackbox">
	<h2><?echo I18N("h","ADD INBOUND FILTER RULE");?></h2>
	<table align="center">
	<tr>
		<td align="right"><?echo I18N("h","Name");?>&nbsp;:</td>
		<td><input type="text" id="inbfdesc" size=20 maxlength="15"></td>
	</tr>
	<tr>
		<td align="right"><?echo I18N("h","Action");?>&nbsp;:</td>
		<td>
			<select id="inbfact">
				<option value="allow"><?echo I18N("h","Allow");?></option>
				<option value="deny"><?echo I18N("h","Deny");?></option>
			</select>
		</td>
	</tr>	
	<tr>
		<td align="right"><?echo I18N("h","Remote IP Range");?>&nbsp;:</td>
		<td>
			&nbsp;<?echo I18N("h","Enable");?>&nbsp;
			<?echo I18N("h","Remote IP Start");?>&nbsp;&nbsp;&nbsp;&nbsp;
			<?echo I18N("h","Remote IP End");?>&nbsp;
		</td>
	</tr>
<?
$INDEX = 1;
while ($INDEX <= 8)
{	
	echo	"	<tr>"."\n";
	echo	"		<td></td>"."\n";
	echo	"		<td>"."\n";
	echo	"			&nbsp;&nbsp;&nbsp;<input type='checkbox' id='en_inbf".$INDEX."'>&nbsp;&nbsp;"."\n";
	echo	"			<input type='text' id='inbf_startip".$INDEX."' value='0.0.0.0' size=16 maxlength='15'>"."\n";
	echo	"			<input type='text' id='inbf_endip".$INDEX."' value='255.255.255.255' size=16 maxlength='15'>"."\n";
	echo	"		</td>"."\n";	
	echo	"	</tr>"."\n";
	$INDEX++;
}			
?>	
	</table>	
	<div class="centerline">
		<input type="button" id="inbfsubmit" value="<?echo I18N("h","Add");?>" onclick="PAGE.OnClickInbFSubmit();">
		<input type="button" id="inbfcancel" value="<?echo I18N("h","Cancel");?>" onclick="PAGE.OnClickInbFCancel();">
	</div>
</div>
<div class="blackbox">
	<h2><?echo I18N("h","INBOUND FILTER RULES LIST");?></h2>
	<table id="inbftable" class="general">
		<tr>
			<th width="30px"><?echo I18N("h","Name");?></th>
			<th width="30px"><?echo I18N("h","Action");?></th>
			<th width="200px"><?echo I18N("h","Remote IP Range");?></th>
			<th width="15px"> </th>
			<th width="15px"> </th>
		</tr>
	</table>
	<div class="gap"></div>
</div>
</form>
