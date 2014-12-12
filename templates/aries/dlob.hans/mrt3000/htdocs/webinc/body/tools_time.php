	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Time and Date");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="2" class="rc_gray5_hd"><h2><?echo I18N("h","Time and Date");?></h2></th>
		</tr>
        <tr>
			<td colspan="2" class="gray_bg border_2side"><cite><?echo I18N("h","The Time and Date Configuration option allows you to configure, update, and maintain the correct time on the internal system clock.")." ".
		I18N("h","In this section you can set the time zone you are in and set the NTP (Network Time Protocol) Server.")." ".
		I18N("h","Daylight Saving can also be configured to adjust the time when needed.");?></cite>
        	</td>
        </tr>
        
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Time and Date Configuration");?></p></td>
        </tr>
        <tr>
			<td width="24%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Time");?> :</td>
			<td width="76%" class="gray_bg border_right">
				<span class="value" id="st_time"></span>
			</td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Time Zone");?> :</td>
			<td class="gray_bg border_right">
				<span class="value">
				<select id="timezone" class="tzselect" style="width:98%" onchange="PAGE.SelectTimeZone(true);">
<?
				foreach ("/runtime/services/timezone/zone")
				echo '\t\t\t<option value="'.$InDeX.'">'.get("h","name").'</option>\n';
?>
				</select>
				</span>
			</td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Enable Daylight Saving");?> :</td>
			<td class="gray_bg border_right">
				<span class="value">
				<input type="checkbox"	id="daylight" onclick="PAGE.DaylightSetEnable();"/>&nbsp;			
				</span>
			</td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Daylight Saving Offset");?> :</td>
			<td class="gray_bg border_right">
				<span class="value">
				<select id="daylight_offset">								
	            <option value="-02:00">-02:00</option>
	            <option value="-01:30">-01:30</option>
	            <option value="-01:00">-01:00</option>
	            <option value="-00:30">-00:30</option>
	            <option value="+00:30">+00:30</option>
	            <option value="+01:00">+01:00</option>
	            <option value="+01:30">+01:30</option>
	            <option value="+02:00">+02:00</option>
	    		</select>	    	
				</span>
			</td>
		</tr>
        <tr>
			<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Daylight Saving Dates");?> :</td>
			<td class="gray_bg border_right">
        	<span class="value">
			<table>
				<tbody>
				<tr align="center">
					<td>&nbsp;</td>
					<td>Month</td>
					<td>Week</td>
					<td>Day of Week</td>
					<td>Time</td>
				</tr>
				<tr align="center">
					<td align="left">DST Start&nbsp;</td>
					<td>
						<select id="daylight_sm">							
							<option value=1><? echo I18N("h","Jan"); ?></option>
							<option value=2><? echo I18N("h","Feb"); ?></option>
							<option value=3><? echo I18N("h","Mar"); ?></option>
							<option value=4><? echo I18N("h","Apr"); ?></option>
							<option value=5><? echo I18N("h","May"); ?></option>
							<option value=6><? echo I18N("h","Jun"); ?></option>
							<option value=7><? echo I18N("h","Jul"); ?></option>
							<option value=8><? echo I18N("h","Aug"); ?></option>
							<option value=9><? echo I18N("h","Sep"); ?></option>
							<option value=10><? echo I18N("h","Oct"); ?></option>
							<option value=11><? echo I18N("h","Nov"); ?></option>
							<option value=12><? echo I18N("h","Dec"); ?></option>
						</select>
					</td>
					<td>
						<select id="daylight_sw">							
							<option value=1><? echo I18N("h","1st"); ?></option>
							<option value=2><? echo I18N("h","2nd"); ?></option>
							<option value=3><? echo I18N("h","3rd"); ?></option>
							<option value=4><? echo I18N("h","4th"); ?></option>
							<option value=5><? echo I18N("h","5th"); ?></option>							
						</select>
					</td>
					<td>
						<select id="daylight_sd">							
							<option value=0><? echo I18N("h","Sun"); ?></option>
							<option value=1><? echo I18N("h","Mon"); ?></option>
							<option value=2><? echo I18N("h","Tue"); ?></option>
							<option value=3><? echo I18N("h","Wed"); ?></option>
							<option value=4><? echo I18N("h","Thu"); ?></option>
							<option value=5><? echo I18N("h","Fri"); ?></option>
							<option value=6><? echo I18N("h","Sat"); ?></option>
						</select>
					</td>
					<td>											
						<select id="daylight_st">
							<script language="javascript">							
								PAGE.DayLightTimeObj();
							</script>
						</select>						
					</td>
				</tr>					
				<tr align="center">
					<td align="left">DST End&nbsp;</td>
					<td>
						<select id="daylight_em">							
							<option value=1><? echo I18N("h","Jan"); ?></option>
							<option value=2><? echo I18N("h","Feb"); ?></option>
							<option value=3><? echo I18N("h","Mar"); ?></option>
							<option value=4><? echo I18N("h","Apr"); ?></option>
							<option value=5><? echo I18N("h","May"); ?></option>
							<option value=6><? echo I18N("h","Jun"); ?></option>
							<option value=7><? echo I18N("h","Jul"); ?></option>
							<option value=8><? echo I18N("h","Aug"); ?></option>
							<option value=9><? echo I18N("h","Sep"); ?></option>
							<option value=10><? echo I18N("h","Oct"); ?></option>
							<option value=11><? echo I18N("h","Nov"); ?></option>
							<option value=12><? echo I18N("h","Dec"); ?></option>
						</select>
					</td>
					<td>
						<select id="daylight_ew">							
							<option value=1><? echo I18N("h","1st"); ?></option>
							<option value=2><? echo I18N("h","2nd"); ?></option>
							<option value=3><? echo I18N("h","3rd"); ?></option>
							<option value=4><? echo I18N("h","4th"); ?></option>
							<option value=5><? echo I18N("h","5th"); ?></option>							
						</select>
					</td>
					<td>
						<select id="daylight_ed">							
							<option value=0><? echo I18N("h","Sun"); ?></option>
							<option value=1><? echo I18N("h","Mon"); ?></option>
							<option value=2><? echo I18N("h","Tue"); ?></option>
							<option value=3><? echo I18N("h","Wed"); ?></option>
							<option value=4><? echo I18N("h","Thu"); ?></option>
							<option value=5><? echo I18N("h","Fri"); ?></option>
							<option value=6><? echo I18N("h","Sat"); ?></option>
						</select>
					</td>
					<td>
						<select id="daylight_et">
							<script language="javascript">							
									PAGE.DayLightTimeObj();
							</script>
						</select>		
					</td>
				</tr>
				</tbody>
			</table>
			</span>
			</td>
		</tr>
        
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Automatic Time and Date Configuration");?></p></td>
        </tr>
        <tr>
        	<td colspan="2" class="gray_bg border_right"><cite>
            	<input name="ntp_enable" id="ntp_enable" onclick="PAGE.OnClickNtpEnb();" type="checkbox"><?echo I18N("h","Automatically synchronize with one of the Internet Time Servers below");?></cite>
            </td>
		</tr>
        <tr>
        	<td nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","NTP Server Used");?> :</td>
			<td class="gray_bg border_right">
				<span class="value">
				<select id="ntp_server">
				<option value=""><?echo I18N("h","Select NTP Server");?></option>
				<option value="asia.pool.ntp.org">asia.pool.ntp.org</option>
				<option value="tw.pool.ntp.org">tw.pool.ntp.org</option>
				</select>
				<input id="ntp_sync" type="button" value="<?echo I18N("h","Update Now");?>" onclick="PAGE.OnClickNTPSync()">
                <?drawlabel("ntp_server");?>
				</span>
			</td>
		</tr>
        <tr>
			<td colspan="2" class="gray_bg border_right">
				<cite><div id=sync_msg></div></cite>
			</td>
		</tr>
                
        <tr>
        	<td colspan="2" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","Set the Time and Date Manually");?></p></td>
        </tr>
        <tr>
        	<td colspan="2" nowrap="nowrap" class="td_right gray_bg border_left"><cite>
            <table width="50%" border="0" align="left" cellpadding="0" cellspacing="0" class="setup_form" style="margin:0;">
            <tr>
			<td width="10%"><?echo I18N("h","Year");?></td>
			<td width="20%">
				<select id="year" onchange="PAGE.OnChangeYear()">
		  		<?
					$i=2008;
					while ($i<2022) { $i++; echo "<option value=".$i.">".$i."</option>\n"; }

		  		?>
                </select>
			</td>
            <td width="10%"><?echo I18N("h","Month");?></td>
			<td width="20%">
				<select id="month" onchange="PAGE.OnChangeMonth()">
				<option value=1><? echo I18N("h","Jan"); ?></option>
				<option value=2><? echo I18N("h","Feb"); ?></option>
				<option value=3><? echo I18N("h","Mar"); ?></option>
				<option value=4><? echo I18N("h","Apr"); ?></option>
				<option value=5><? echo I18N("h","May"); ?></option>
				<option value=6><? echo I18N("h","Jun"); ?></option>
				<option value=7><? echo I18N("h","Jul"); ?></option>
				<option value=8><? echo I18N("h","Aug"); ?></option>
				<option value=9><? echo I18N("h","Sep"); ?></option>
				<option value=10><? echo I18N("h","Oct"); ?></option>
				<option value=11><? echo I18N("h","Nov"); ?></option>
				<option value=12><? echo I18N("h","Dec"); ?></option>
				</select>
			</td>
			<td width="10%"><?echo I18N("h","Day");?></td>
			<td width="20%">
				<select id="day"></select>
			</td>
			</tr>
        	<tr>
			<td width="10%"><?echo I18N("h","Hour");?></td>
			<td width="20%">
				<select id="hour"><?
				$i=0;
				while ($i<24) { echo "<option value=".$i.">".$i."</option>\n"; $i++; }
			?></select>
			</td>
            <td width="10%"><?echo I18N("h","Minute");?></td>
			<td width="20%">
				<select id="minute"><?
				$i=0;
				while ($i<60) { echo "<option value=".$i.">".$i."</option>\n"; $i++; }
			?></select>
			</td>
            <td width="10%"><?echo I18N("h","Second");?></td>
			<td width="20%">
				<select id="second"<?
				$i=0;
				while ($i<60) { echo "<option value=".$i.">".$i."</option>\n"; $i++; }
			?></select>
			</td>
            </tr>
            <tr>
			<td colspan="6" nowrap="nowrap">	
				<input type="button" id="manual_sync" <? if(query("/runtime/device/langcode")!="en") echo 'style="width: 88%;"';?> value="<?echo I18N("h","Sync. your computer's time settings");?>" onclick="PAGE.onClickManualSync();">
            </td>
       		</tr>
            </table></cite>
            </td>
       	</tr>
        
        <tr>
			<td colspan="2" nowrap="nowrap" class="gray_bg border_left" id=sync_pc_msg></td>
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
