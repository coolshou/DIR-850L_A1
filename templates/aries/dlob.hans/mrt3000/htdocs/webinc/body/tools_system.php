<style type="text/css">
span.name_l
{
	width: 45%;
	float: left;
	text-align: right;
	font-weight: bold;
	margin-top: 4px;
	font-size:12px;
	color:#000000;
}
span.delimiter
{
	width: 3%;
	float: left;
	text-align: center;
	font-weight: bold;
	margin-top: 4px;
	font-size:12px;
	color:#000000;
}
.gradient_form_content { margin:10px auto auto 0px; width:800px; font-size:15px; color:#333;}
.gradient_form_content span {display: inline-block;}
</style>
<div id="content" class="maincolumn">
<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","System");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a><br />
	<div class="rc_gradient_hd" style="background:url(../pic/tag_gradient_hd.gif) 0 -100px repeat-x;">
		<h2><?echo I18N("h","Save and Restore Settings");?></h2>
	</div>

	<div class="rc_gradient_bd h_initial" style="background-color:#f4f4f4;">
		<h6><?echo I18N("h","Once the router is configured you can save the configuration settings to a configuration file on your hard drive. You also have the option to load configuration settings, or restore the factory default settings.");?></h6>

	
		<p class="assistant"><?echo I18N("h","Save and Restore Settings");?></p>
    	<div class="gradient_form_content">
		<form id="dlcfgbin" action="dlcfg.cgi" method="post">
			<span class="name_l"><?echo I18N("h","Save Settings To Local Hard Drive");?></span>
			<span class="delimiter">:</span>
			<span>
				<input type="button" value="<?echo I18N("h","Save Configuration");?>" onClick="PAGE.OnClickDownload();" />
			</span>
		</form>
        <br/>
		<form id="ulcfgbin" action="seama.cgi" method="post" enctype="multipart/form-data">
			<span class="name_l"><?echo I18N("h","Load Settings From Local Hard Drive");?></span>
			<span class="delimiter">:</span>
			<span>
				<input type="hidden" name="REPORT_METHOD" value="301" />
				<input type="hidden" name="REPORT" value="tools_sys_ulcfg.php" />
				<input type="file" id="ulcfg" name="sealpac" size="20" /><?drawlabel("ulcfg");?>
			</span>
			<span class="name_l"></span>
			<span class="delimiter"></span>
			<span>
				<input type="button" value="<?echo I18N("h","Restore Configuration");?>" onClick="PAGE.OnClickUpload();" />
			</span>
		</form>
		<br/>
		<form>
			<span class="name_l"><?echo I18N("h","Restore To Factory Default Settings");?></span>
			<span class="delimiter">:</span>
			<span>
				<input type="button" value="<?echo I18N("h","Restore Factory Defaults");?>" onClick="PAGE.OnClickFReset();" />
			</span>
		</form>
		<br/>
		<form>
			<span class="name_l"><?echo I18N("h","Reboot The Device");?></span>
			<span class="delimiter">:</span>
			<span>
				<input type="button" value="<?echo I18N("h","Reboot the Device");?>" onClick="PAGE.OnClickReboot();" />
			</span>
		</form>
		<br/>
		<form action="tools_fw_rlt.php" method="post">
			<div class="textinput" <?if ($FEATURE_NOLANGPACK=="1") echo ' style="display:none;"';?> >
			<input type="hidden" name="ACTION" value="langclear" />
			<span class="name_l"><?echo i18n("Clear Language Pack");?></span>
			<span class="delimiter">:</span>
			<span>
				<input type="submit" value="<?echo i18n("Clear");?>" />
			</span>
			</div>
		</form>
        </div>
	</div>
    <table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
        	<td colspan="2" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
   	</table>
</div>
