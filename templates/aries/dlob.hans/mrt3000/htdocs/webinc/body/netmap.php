	<div id="content" class="maincolumn">
		<a href="./st_device.php" class="icon list_report" title="List information"><?echo I18N("h","Status");?></a>
		<h1>Network Map</h1> 
		<ul id="connected" class="netmorkMap_picture" style="background:url(../pic/netmorkmap_connection.gif) no-repeat;">
			<li class="device_s"><img src="../pic/netmorkmap_device.gif" /></li>
			<li class="firmware">
			<?echo query("/runtime/device/vendor").' '.query("/runtime/device/modelname");?><br/ >
				<?echo I18N("h","Firmware").':'.query("/runtime/device/firmwareversion");?>
			</li>
			<li class="connection"><?echo I18N("h","Internet");?></li>
			<i></i>
		</ul>   
		<div class="rc_map">
			<table id="leases_list" width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="setup_form" style="margin:0;">
			<tr>
				<td width="8%">&nbsp;</td>
				<td width="31%"><strong><?echo I18N("h","Name");?></strong></td>
				<td width="30%"><strong><?echo I18N("h","IP Address");?></strong></td>
				<td width="31%"><strong><?echo I18N("h","MAC");?></strong></td>
			</tr>
			</table>
		</div>
	</div>
