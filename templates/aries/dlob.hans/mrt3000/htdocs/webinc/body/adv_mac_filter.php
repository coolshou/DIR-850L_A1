<? include "/htdocs/webinc/body/draw_elements.php"; ?>
<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","MAC Address Filtering");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
			<tr>
				<th colspan="<?if ($FEATURE_NOSCH=="1"){echo '4';}else{echo '5';}?>" class="rc_gray5_hd"><h2><?echo I18N("h","MAC Address Filter");?></h2></th>
			</tr>
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '4';}else{echo '5';}?>" class="gray_bg border_2side">
					<cite>
						<?echo I18N("h","The MAC (Media Access Controller) Address filter option is used to restrict network access of your devices based on their MAC Addresses.");?>
						<?echo I18N("h","A MAC address is a unique ID assigned by the manufacturer of the network device.");?>
						<?echo I18N("h","This feature can be configured to ALLOW or DENY network/Internet access.");?>
					</cite>
				</td>
			</tr>

<!-- MAC Filtering START -->
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '4';}else{echo '5';}?>" class="gray_bg border_2side"><p class="subitem"><?=$MAC_FILTER_MAX_COUNT?> -- <?echo I18N("h","MAC Filtering Rules");?></p></td>
			</tr>
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '4';}else{echo '5';}?>" class="gray_bg border_2side">
					<cite>
						<?echo I18N("h","Configure MAC Filtering below:");?>
						<select id="mode" onchange="PAGE.OnChangeMode();">
							<option value="DISABLE"><?echo I18N("h","Turn MAC Filtering OFF");?></option>
							<option value="DROP"><?echo I18N("h","Turn MAC Filtering ON and ALLOW computers listed to access the network");?></option>
							<option value="ACCEPT"><?echo I18N("h","Turn MAC Filtering ON and DENY computers listed to access the network");?></option>
						</select>
					</cite>
				</td>
			</tr>
			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '4';}else{echo '5';}?>" class="gray_bg border_2side">
					<cite>
						<?echo I18N("h","Remaining number of rules that can be created");?>: <span id="rmd" style="color:red;">
					</cite>
				</td>
			</tr>
			<tr>
				<td width="23px" class="gray_bg border_left">&nbsp;</td>
				<td width="133px" class="gray_bg"><b><?echo I18N("h","MAC Address");?></b></td>
				<td width="29px" class="gray_bg">&nbsp;</td>
				<td width="136px" class="gray_bg<?if ($FEATURE_NOSCH=="1"){echo ' border_right';}?>"><b><?echo I18N("h","DHCP Client List");?></b></td>
				<?if ($FEATURE_NOSCH!="1"){echo '<td width="188px" class="gray_bg border_right"><b>'.I18N("h","Schedule").'</b></td>\n';}?>
			</tr>
<?
$INDEX = 1;
while ($INDEX <= $MAC_FILTER_MAX_COUNT) {	dophp("load", "/htdocs/webinc/body/adv_mac_filter_list.php");	$INDEX++; }
?>

			<tr>
				<td colspan="<?if ($FEATURE_NOSCH=="1"){echo '4';}else{echo '5';}?>" class="rc_gray5_ft">
					<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
					<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
				</td>
			</tr>
		</table>
	</div>
</form>
