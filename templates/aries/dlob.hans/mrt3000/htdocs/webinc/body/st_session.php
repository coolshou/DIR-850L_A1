<? include "/htdocs/webinc/libdraw.php"; ?>
<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Internet Sessions");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>

<!--      old style
			<table width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="setup_form" style="margin:0;">
				<tr>
					<th colspan="7" class="rc_gray5_hd"><h2><?echo I18N("h","Internet Sessions");?></h2></th>
				</tr>
				<tr>
					<th colspan="7" class="gray_bg border_right"><p><input type="button" value="<?echo I18N("h","Refresh");?>" onClick="PAGE.OnClickRefresh();" /></p></th>
				</tr>
				<tr>
					<table id="session_list" border="0" cellpadding="0" cellspacing="0" class="status_report">
						<tr>
							<td width="20%"><?echo I18N("h","Local");?></td>
							<td width="10%"><strong><?echo I18N("h","NAT");?></strong></td>
							<td width="20%"><strong><?echo I18N("h","Internet");?></strong></td>
							<td width="13%"><strong><?echo I18N("h","Protocol");?></strong></td>    
							<td width="13%"><strong><?echo I18N("h","State");?></strong></td>
							<td width="10%"><strong><?echo I18N("h","Dir");?></strong></td>
							<td width="14%"><strong><?echo I18N("h","Timeout");?></strong></td></tr>		
						</tr>
					</table>
				</tr>
			</table>                                                                                                               -->
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
			<tr>
				<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Internet Sessions");?></h2></th>
			</tr>
			<tr>
				<th colspan="2" class="gray_bg border_2side"><cite><?echo I18N("h","This page displays Source and Destination sessions passing through the device.");?></cite></th>
			</tr>
<!--
			<tr>                                            
				<td width="100%" class="gray_bg border_right"><button value="submit" class="submitBtn floatLeft" onclick="PAGE.OnClickRefresh();"><b><?echo I18N("h","Refresh");?></b></button></td>
			</tr>
-->
			<tr>
				<td width="100%" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","NAPT Sessions");?></p></td>
			</tr>
			<tr>
				<td width="100%" class="gray_bg border_2side"><div id="sess_total"></div></td>
			</tr>
			<tr>
				<td width="100%" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","NAPT Active Sessions");?></p></td>
			</tr>
			<tr>
				<td width="100%" class="gray_bg border_2side"><span class="name" id="loading"></span></td>
			</tr>
			<tr>
				<td width="100%" class="gray_bg border_2side"><div id="sess_list"></div></td>
			</tr>
			<tr>
				<td class="rc_gray5_ft">
					<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
					<button value="submit" class="submitBtn floatRight" onclick="PAGE.OnClickRefresh();"><b><?echo I18N("h","Refresh");?></b></button>
				</td>
			</tr>
		</table>      
	</div>
</form>
