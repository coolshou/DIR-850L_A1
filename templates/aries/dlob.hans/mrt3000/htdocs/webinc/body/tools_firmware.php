	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Firmware Update");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Firmware Update");?></h2></th>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite>
					<?echo I18N("h","There may be new firmware available that enhances your router's functionality and performance.");?><br/><a href="http://help.miiicasa.com" target="_blank" style="color: #0000FF; text-decoration:underline;"><?echo I18N("h","Click here to check for an upgrade on our support site.");?></a>
					<?echo I18N("h","To upgrade the firmware, locate the upgrade file on the local hard drive with the Browse button. Once you have found the file, click the Upload button to start the firmware upgrade.");?><br />
					<?echo I18N("h","The language pack allows you to change the language of the user interface on the router. We suggest that you upgrade your current language pack if you upgrade the firmware. This ensures that any changes in the firmware are displayed correctly.");?><br />
					<?echo I18N("h","To upgrade the language pack, locate the upgrade file on the local hard drive with the Browse button. Once you have found the file, click the Upload button to start the language pack upgrade.");?>
				</cite>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Firmware Information");?></p></td>
		</tr>
		<tr>
			<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Current Firmware Version");?> :</td>
			<td width="76%" class="gray_bg border_right"><?echo query("/runtime/device/firmwareversion");?></td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Current Firmware Date");?> : </td>
			<td class="gray_bg border_right">
            	<?echo query("/runtime/device/firmwarebuilddate");?>
			</td>
		</tr>
		<tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Check Online Now for Latest Firmware Version");?> : </td>
			<td class="gray_bg border_right">
				<input type="button" id="chkfw_btn" value='<?echo I18N("h","Check Now");?>' onclick="PAGE.OnClickChkFW();" />
			</td>
		</tr>
        <tr>
			<td colspan="2" class="gray_bg border_2side"><p id="fw_message" style="text-align:center; font-weight:bold; display:none;"></P></td>
        </tr>
        
        <!---->        
        	<tr style="display:none;" id="tr_ckfwver" name="tr_ckfwver">
				<td class="gray_bg border_2side" colspan="2"><p class="subitem"><?echo I18N("h","Check Latest Firmware Version");?></p></td>
			</tr>
			<tr style="display:none;" id="tr_ckfwver" name="tr_ckfwver">
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Latest Firmware Version");?> :</td>
				<td class="gray_bg border_right"><span class="value" id=latest_fw_ver></span></td>
			</tr>
        	<tr style="display:none;" id="tr_ckfwver" name="tr_ckfwver">
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Latest Firmware Date");?> :</td>
				<td class="gray_bg border_right"><span class="value" id=latest_fw_date></span></td>
			</tr>
        	<tr style="display:none;" id="tr_ckfwver" name="tr_ckfwver">
				<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Available Download Site");?> :</td>
				<td class="gray_bg border_right"><select id=fw_dl_locs size=1></select></td>
			</tr>
        	<tr style="display:none;" id="tr_ckfwver" name="tr_ckfwver">
        		<td nowrap="nowrap" class="td_right gray_bg border_left"></td>
				<td class="gray_bg border_right"><input type="button" id="chkfw_btn" value='<?echo I18N("h","Download");?>' onclick="PAGE.OnClickDownloadFW();" /></td>
			</tr>

        <!---->  
        <!---------Firmware Upgrade START--------->  
        <form id="fwup" action="fwup.cgi" method="post" enctype="multipart/form-data">
        <tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Firmware Upgrade");?></p></td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite>
                <strong><font style="color:red">
				<?echo I18N("h","Note: Some firmware upgrades reset the configuration options to the factory defaults. Before performing an upgrade, be sure to save the current configuration.");?></font><br />
                <font style="color:blue">
				<?echo I18N("h","To upgrade the firmware, your PC must have a wired connection to the router. Enter the name of the firmware upgrade file, and click on the Upload button.");?></font>
                </strong>
				<input type="hidden" name="REPORT_METHOD" value="301" />
				<input type="hidden" name="REPORT" value="tools_fw_rlt.php" />
				<input type="hidden" name="DELAY" value="10" />
				<input type="hidden" name="PELOTA_ACTION" value="fwupdate" />
				</cite>
			</td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Upload");?> :</td>
			<td class="gray_bg border_right"><input type="file" name="fw" size=40 />
			<input type="submit" value="<?echo I18N("h","Upload");?>" /></span></td>
		</tr> 
        </form>  
        <!---------Firmware Upgrade END--------->  
        <!---------Language Pack Upgrade START--------->    
		<form id="langup" action="tools_fw_rlt.php" method="post" enctype="multipart/form-data">
        <tr>
			<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Language Pack Upgrade");?></p></td>
		</tr>
		<tr>
			<td colspan="2" class="gray_bg border_2side">
				<cite>
				<input type="hidden" name="ACTION" value="langupdate" />
				</cite>
			</td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Upload");?> :</td>
			<td class="gray_bg border_right"><input type="file" name="sealpac" size=40 />
			<input type="submit" value="<?echo I18N("h","Upload");?>" /></span></td>
		</tr>
        <tr>
			<td colspan="2" class="rc_gray5_ft">
				<button type="button" value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
        </form>
        <!---------Language Pack Upgrade END---------> 
        </table>

        </div>
