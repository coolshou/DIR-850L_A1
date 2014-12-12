	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Wireless");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Connected Wireless Client List");?></h2></th>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h","View the wireless clients that are connected to the router.");?>
					(<?echo I18N("h","A client might linger in the list for a few minutes after an unexpected disconnect.");?>)
				</cite>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<p class="subitem"><?echo I18N("h","Number Of Wireless Clients");?> : <span id="client_cnt"></span></p>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<div class="rc_map">
				<table id="client_list" width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="setup_form" style="margin:0;">
				<tr>
					<td width="35%"><strong><?echo I18N("h","MAC Address");?></strong></td>
					<td width="30%"><strong><?echo I18N("h","IP Address");?></strong></td>
					<td width="15%"><strong><?echo I18N("h","Mode");?></strong></td>
					<td width="20%"><strong><?echo I18N("h","Rate");?> (Mbps)</strong></td>
				</tr>
				</table>
				</div>
			</td>
		</tr>
		<tr id="div_5G" name="div_5G" style="display:none;">
			<td colspan="2" class="gray_bg border_2side">
				<p class="subitem"><?echo I18N("h","Number Of Wireless Clients - 5GHz Band");?> : <span id="client_cnt_Aband"></span></p>
			</td>
		</tr>
		<tr id="div_5G" name="div_5G" style="display:none;">
			<td colspan="2" class="gray_bg border_2side">
				<div class="rc_map">
				<table id="client_list_Aband" width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="setup_form" style="margin:0;">
				<tr>
					<td width="35%"><strong><?echo I18N("h","MAC Address");?></strong></td>
					<td width="30%"><strong><?echo I18N("h","IP Address");?></strong></td>
					<td width="15%"><strong><?echo I18N("h","Mode");?></strong></td>
					<td width="20%"><strong><?echo I18N("h","Rate");?> (Mbps)</strong></td>
				</tr>
				</table>
				</div>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatRight" onclick="BODY.OnReload();"><b><?echo I18N("h","Refresh");?></b></button>
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
		</table>
	</div>
	</form>
