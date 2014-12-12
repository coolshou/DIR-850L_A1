	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Logs");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","View Log");?></h2></th>
		</tr>
        <tr>
			<td colspan="2" class="gray_bg border_2side"><cite><?echo I18N("h","The View Log displays the activities occurring on the")." ";echo query("/runtime/device/modelname");?>.</cite></td>
		</tr>
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Save Log File");?></p></td>
        </tr>
        <tr>
        	<td colspan="2" nowrap="nowrap" class="gray_bg border_2side"><cite><?echo I18N("h","Save Log File To Local Hard Drive.");?><input name="save_log" value="<?echo I18N("h","Save");?>" onclick="window.location.href='/log_get.php';" type="button"/></cite>
            </td>
        </tr>
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Log Type & Level");?></p></td>
        </tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Log Type");?>:</td>
			<td class="gray_bg border_right">
            	<input type="radio" id="sysact" name="Type" onclick='PAGE.OnClickChangeType("sysact");' modified="ignore"><label for="system"><?echo I18N("h","System");?></label>
                <input type="radio" id="attack" name="Type" onclick='PAGE.OnClickChangeType("attack");' modified="ignore"><label for="firewallsecurity"><?echo I18N("h","Firewall & Security");?></label>
                <input type="radio" id="drop" name="Type" onclick='PAGE.OnClickChangeType("drop");' modified="ignore"><label for="routerstatus"><?echo I18N("h","Router Status");?></label>
           	</td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Log Level");?>:</td>
			<td class="gray_bg border_right">
            	<input type="radio" id="LOG_dbg" name="Level"><label for="critical"><?echo I18N("h","Critical");?></label>
                <input type="radio" id="LOG_warn" name="Level"><label for="warning"><?echo I18N("h","Warning");?></label>
                <input type="radio" id="LOG_info" name="Level"><label for="information"><?echo I18N("h","Information");?></label>
            </td>
		</tr>
    
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Log Files");?></p></td>
        </tr>
        
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><cite>
            	<input type=button value="<?echo I18N("h","First Page");?>" id="fp"  onclick="PAGE.OnClickToPage('1')">
				<input type=button value="<?echo I18N("h","Last Page");?>" id="lp"  onclick="PAGE.OnClickToPage('0')">
				<input type=button value="<?echo I18N("h","Previous");?>" id="pp"  onclick="PAGE.OnClickToPage('-1')">
				<input type=button value="<?echo I18N("h","Next");?>" id="np" onclick="PAGE.OnClickToPage('+1')">
				<input type=button value="<?echo I18N("h","Clear");?>" id="clear" onclick="PAGE.OnClickClear()">
				<input type=button value='<?echo I18N("h","Link To Email Log Settings");?>' onclick="javascript:self.location.href='tools_email.php'"></cite>
            </td>
        </tr>
        <tr>
        	<td colspan="2" nowrap="nowrap" class="gray_bg border_2side">
            	<cite><div id="sLog_page"></div></cite>
			</td>
        </tr>
        <tr>
        	<td colspan="2" nowrap="nowrap" class="gray_bg border_2side">
            	<cite><div class="rc_map" id="sLog" style="width:100%"></div></cite>
			</td>
        </tr>
		<tr>
			<td colspan="2" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
		</table>
	</div>
	</form>
