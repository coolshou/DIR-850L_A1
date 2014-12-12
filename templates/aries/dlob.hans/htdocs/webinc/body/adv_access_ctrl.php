<?
function wiz_buttons()
{
	echo '<div class="emptyline"></div>\n'.
		 '	<div class="centerline">\n'.
		 '		<input type="button" name="b_prev" value="'.i18n("Prev").'" onClick="PAGE.OnClickPrev();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_next" value="'.i18n("Next").'" onClick="PAGE.OnClickNext();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_save" value="'.i18n("Save").'" onClick="PAGE.OnClickSave();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_cancel" value="'.i18n("Cancel").'" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;\n'.
		 '	</div>\n'.
		 '	<div class="emptyline"></div>';
}
?>
<form id="mainform" onsubmit="return false;">
<div id="access_main">	
	<div class="orangebox">
		<h1><?echo i18n("Access Control");?></h1>
		<p><?echo i18n("The Access Control option allows you to control access in and out of your network. Use this feature as Access Controls to only grant access to approved sites, limit web access based on time or dates, and/or block internet access for applications like P2P utilities or games.");?></p>
		<p><input type="button" value="<?echo i18n("Save Settings");?>" onClick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onClick="BODY.OnReload();" /></p>
	</div>
	<div class="blackbox">
		<h2><?echo i18n("ACCESS CONTROL");?></h2>
		<div class="textinput">
			<span class="name"><?echo i18n("Enable Access Control");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input id="en_access" type="checkbox" onClick="PAGE.OnClickEnACCESS();" />
			</span>
		</div>
		<div class="textinput">
			<span class="name"></span>
			<span class="delimiter"></span>
			<span class="value">
				<input id="add_policy" type="button" value="<?echo i18n("Add Policy");?>" onClick="PAGE.OnClickAddPolicy();" />
			</span>
		</div>
		<div class="emptyline"></div>
	</div>
	<div id="policytableframe" class="blackbox">
		<h2><?echo i18n("POLICY TABLE");?></h2>
		<table id="policytable" class="general">
			<tr>
				<th width="30px"><?echo i18n("Enable");?></th>
				<th width="40px"><?echo i18n("Policy");?></th>
				<th width="65px"><?echo i18n("Machine");?></th>
				<th width="50px"><?echo i18n("Filtering");?></th>
				<th width="30px"><?echo i18n("Logged");?></th>
				<th width="30px"><?echo i18n("Schedule");?></th>
				<th width="15px"> </th>
				<th width="15px"> </th>
			</tr>			
		</table>
		<div class="gap"></div>
	</div>
	<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</div>		

<div id="access_descript" style="display:none;">
	<div class="blackbox">
		<h2><?echo i18n("ADD NEW POLICY");?></h2>
		<p class="strong"><?echo i18n("This wizard will guide you through the following steps to add a new policy for Access Control.");?></p>
		<p><?echo i18n("Step 1 - Choose a unique name for your policy");?></p>
		<p><?echo i18n("Step 2 - Select a schedule");?></p>
		<p><?echo i18n("Step 3 - Select the machine to which this policy applies");?></p>
		<p><?echo i18n("Step 4 - Select filtering method");?></p>
		<p><?echo i18n("Step 5 - Select filters");?></p>
		<p><?echo i18n("Step 6 - Configure Web Access Logging");?></p>
		<?wiz_buttons();?>
	</div>		
</div>		

<div id="access_name" style="display:none;">
	<div class="blackbox">
		<h2><?echo i18n("STEP 1: CHOOSE POLICY NAME");?></h2>
		<p class="strong"><?echo i18n("Choose a unique name for your policy.");?></p>
		<div class="textinput">
			<span class="name"><?echo i18n("Policy Name");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="text" id="policyname" size="20" maxlength="16"></span>
		</div>
		<?wiz_buttons();?>
	</div>		
</div>	

<div id="access_schedule" style="display:none;">
	<div class="blackbox">
		<h2><?echo i18n("STEP 2: SELECT SCHEDULE");?></h2>
		<p class="strong"><?echo i18n("Choose a schedule to apply to this policy.");?></p>
		<div class="centerline" align="center">
			<div class="textinput">
				<span class="name"></span>
				<span class="delimiter"></span>
				<span class="value">
					<select id="sch_select" onChange="PAGE.OnClickSchSelect(this.value)">
						<option value="always"><?echo i18n("Always");?></option>
<?
				foreach ("/schedule/entry")
				{					
				echo '\t\t\t<option value="'.get("x","uid").'">'.get("h","description").'</option>\n';
				}	
?>
					</select>
				</span>
			</div>
			<div class="textinput">
				<span class="name"><?echo i18n("Details");?></span>
				<span class="delimiter">:</span>
				<span class="value"><input type="text" id="sch_detail" maxlength="60" size="40" disabled></span>
			</div>
		</div>
		<?wiz_buttons();?>
	</div>		
</div>	

<div id="access_machine" style="display:none;">
	<div class="blackbox">
		<h2><?echo i18n("STEP 3: SELECT MACHINE");?></h2>
		<p class="strong"><?echo i18n("Select the machine to which this policy applies.");?></p>
		<p><?echo i18n("Specify a machine with its IP or MAC address, or select 'Other Machines' for machines that do not have a policy.");?></p>
		<div class="textinput">
			<span class="name"><?echo i18n("Address Type");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input type="radio" id="MIP" onclick="PAGE.OnClickMachineType('IP');"><?echo i18n("IP");?>
				<input type="radio" id="MMAC" onclick="PAGE.OnClickMachineType('MAC');"><?echo i18n("MAC");?>
				<input type="radio" id="MOthers" onclick="PAGE.OnClickMachineType('Others');"><?echo i18n("Other Machines");?>	
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("IP Address");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input type="text" id="MachineIP" maxlength="15">
				<span>&nbsp;&lt;&lt;&nbsp;</span>
				<select id="MachineIPSelect" onChange="PAGE.OnClickMachineIPSelect(this.value)">
						<option value="">Computer Name</option>
<?
				$p = XNODE_getpathbytarget("/runtime", "inf", "uid", "LAN-1", 0);
				foreach ($p."/dhcps4/leases/entry")
				{
					echo '\t\t\t\t\t\t<option value="'.query("ipaddr").'">'.query("hostname").' ('.query("ipaddr").')</option>\n';
				}
?>		
				</select>				
			</span>			
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Machine Address");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input type="text" id="MachineMAC" maxlength="17">
				<span>&nbsp;&lt;&lt;&nbsp;</span>
				<select id="MachineMACSelect" onChange="PAGE.OnClickMachineMACSelect(this.value)">
						<option value="">Computer Name</option>
<?
				$p = XNODE_getpathbytarget("/runtime", "inf", "uid", "LAN-1", 0);
				foreach ($p."/dhcps4/leases/entry")
				{
					echo '\t\t\t\t\t\t<option value="'.query("macaddr").'">'.query("hostname").' ('.query("macaddr").')</option>\n';
				}
?>		
				</select>				
			</span>			
		</div>		
		<div class="textinput">
			<span class="name"></span>
			<span class="delimiter"></span>
			<span class="value"><input id="ipv4_mac_button" type="button" value="<?echo I18N("h","Clone Your PC's MAC Address");?>" onclick="PAGE.OnClickMACButton();" /></span>
		</div>		
		<div class="textinput">
			<span class="name"></span>
			<span class="delimiter"></span>
			<span class="value">
				<input id="machine_submit" type="button" value="<?echo i18n("Add");?>" onclick="PAGE.OnClickMachineSubmit();" />
				<input id="machine_cancel" type="button" value="<?echo i18n("Cancel");?>" onclick="PAGE.OnClickMachineCancel();" />
			</span>	
		</div>
		<div class="gap"></div>
		<div>
			<table id="machinetable" class="general">
				<tr>
					<th width="570px"><?echo i18n("Machine");?></th>
					<th width="40px"> </th>
					<th width="40px"> </th>
				</tr>
			</table>
		</div>	
		<?wiz_buttons();?>
	</div>		
</div>	

<div id="access_filter_meth" style="display:none;">
	<div class="blackbox">
		<h2><?echo i18n("STEP 4: SELECT FILTERING METHOD");?></h2>
		<p class="strong"><?echo i18n("Select the method for filtering.");?></p>
		<div class="textinput">
			<span class="name"><?echo i18n("Method");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input type="radio" id="LOGWEBONLY" onclick="PAGE.OnClickFilterMethod('LOGWEBONLY');"><?echo i18n("Log Web Access Only");?>
				<input type="radio" id="BLOCKALL" onclick="PAGE.OnClickFilterMethod('BLOCKALL');"><?echo i18n("Block All Access");?>
				<input type="radio" id="BLOCKSOME" onclick="PAGE.OnClickFilterMethod('BLOCKSOME');"><?echo i18n("Block Some Access");?>	
			</span>
		</div>
		<div class="textinput" id="WebFilter" style="display:none;">
			<span class="name"><?echo i18n("Apply Web Filter");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input type="checkbox" id="WebFilterCheck">
			</span>
		</div>
		<div class="textinput" id="PortFilter" style="display:none;">
			<span class="name"><?echo i18n("Apply Advanced Port Filters");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input type="checkbox" id="PortFilterCheck">
			</span>
		</div>
		<?wiz_buttons();?>
	</div>		
</div>

<div id="access_port_filter" style="display:none;">
	<div class="blackbox">
		<h2><?echo i18n("STEP 5: PORT FILTER");?></h2>
		<p class="strong"><?echo i18n("Add Port Filters Rules.");?></p>
		<p><?echo i18n("Specify rules to prohibit access to specific IP addresses and ports.");?></p>
		<div>	
			<table class="general">	
				<tr>
					<td class="centered" width="40px">Enable</td>
					<td class="centered" width="100px">Name</td>
					<td class="centered" width="120px">Dest IP Start</td>
					<td class="centered" width="120px">Dest IP End</td>
					<td class="centered" width="60px">Protocol</td>
					<td class="centered" width="100px">Dest Port Start</td>
					<td class="centered" width="100px">Dest Port End</td>
				</tr>
<?
$INDEX = 1;
while ($INDEX <= 8)
{	
	echo	"<tr>"."\n";
	echo	"<td class='centered'><input type='checkbox' id='filter_enable".$INDEX."'></td>"."\n";
	echo	"<td class='centered'><input type='text' size=14 id='filter_name".$INDEX."' maxlength='15'></td>"."\n";
	echo	"<td class='centered'><input type='text' size=16 id='filter_startip".$INDEX."' value='0.0.0.0' maxlength='15'></td>"."\n";
	echo	"<td class='centered'><input type='text' size=16 id='filter_endip".$INDEX."' value='255.255.255.255' maxlength='15'></td>"."\n";
	echo	"<td class='centered'>"."\n";
	echo		"<select id='filter_protocol".$INDEX."' onChange='PAGE.OnClickProtocol(".$INDEX.")'>"."\n";
	echo		"<option value='ALL'>Any</option>"."\n";
	echo		"<option value='ICMP'>ICMP</option>"."\n";
	echo		"<option value='TCP'>TCP</option>"."\n";
	echo		"<option value='UDP'>UDP</option>"."\n";	
	echo	"</td>"."\n";	
	echo	"<td class='centered'><input type='text' size=10 id='filter_startport".$INDEX."' value='0' maxlength='5'></td>"."\n";
	echo	"<td class='centered'><input type='text' size=10 id='filter_endport".$INDEX."' value='65535' maxlength='5'></td>"."\n";		
	echo	"</tr>";
	$INDEX++;
}			
?>		
			</table>		
		</div>
		<?wiz_buttons();?>
	</div>		
</div>

<div id="access_web_logging" style="display:none;">
	<div class="blackbox">
		<h2><?echo i18n("STEP 6: CONFIGURE WEB ACCESS LOGGING");?></h2>
		<div class="textinput">
			<span class="name"><?echo i18n("Web Access Logging");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<input type="radio" id="WebLogDisabled" onclick="PAGE.OnClickWebLogging('disable');"><?echo i18n("Disabled");?>
			</span>
		</div>
		<div class="textinput">
			<span class="name"></span>
			<span class="delimiter"></span>
			<span class="value">
				<input type="radio" id="WebLogEnabled" onclick="PAGE.OnClickWebLogging('enable');"><?echo i18n("Enabled");?>
			</span>
		</div>
		<?wiz_buttons();?>
	</div>		
</div>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
</form>

