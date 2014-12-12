	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","System Check");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Ping Test");?></h2></th>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite><?echo I18N("h",'Ping Test sends "ping" packets to a device on the Internet to test its accessibility.');?></cite>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Host Name or IP Address");?> :</td>
			<td class="gray_bg border_right">
				<input type="text" id="dst_v4" maxlength="63" size="20" class="text_block" />
				<input type="button" id="ping_v4" value="Ping" onclick='PAGE.OnClick_Ping("_v4");' />
				<?drawlabel("dst_v4");?>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Ping Result");?></p></td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite id="report"><?echo I18N("h","Enter a host name or IP address above and click 'Ping'.");?></cite>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
		</table>
	</div>
	</form>
