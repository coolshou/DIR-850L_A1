<? 
include "/htdocs/webinc/body/draw_elements.php";
include "/htdocs/phplib/xnode.php";
?>
<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","QoS");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>

		<div class="rc_gray5_hd" style="clear:both" >
			<h2 style="padding:10px 20px;"><?echo I18N("h","QoS SETTINGS");?></h2>
			<div class="gray_bg setup_form">
				<p style="padding:10px 20px;">
					<?echo I18N("h","Use this section to configure D-Link's QoS Engine powered by QoS Engine Technology. This QoS Engine improves your online gaming experience by ensuring that your game traffic is prioritized over other network traffic, such as FTP or Web.For best performance, use the Automatic Classification option to automatically set the priority for your applications.");?>
				</p>
				<div class="blackbox">
					<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
						<tr>
							<td colspan="3" class="gray_bg"><p class="subitem"><?echo I18N("h","QoS Setup");?></p></td>
						</tr>                        
						<tr>
							<td width="15%" align="right"><?echo I18N("h","Enable QoS");?></td>
							<td width="3%" align="center">:</td>
							<td width="81%" align="left"><input id="en_qos" type="checkbox" onclick="PAGE.OnClickQOSEnable();"/></td>
						</tr>
						<tr>
							<td width="15%" align="right"><?echo I18N("h","QOS Type");?></td>
							<td width="3%" align="center">:</td>
							<td width="81%" align="left">
								<select name="QoS_Type" id="QoS_Type" onChange="PAGE.OnChangeQoSType('');">
									<option value="lan_port"><?echo I18N("h","Priority By Lan Port");?></option>
									<option value="protocol"><?echo I18N("h","Priority By Protocol");?></option>
								</select>
							</td>
						</tr>
					</table>
					<div id="show_Port_Priority">
						<table width="100%" border="0" cellspacing="0" cellpadding="0" class="adv">            
							<tr>
								<td width="15%" align="right"><?echo I18N("h","Lan Port 1");?></td>
								<td width="3%" align="center">:</td>
								<td width="81%" align="left">
            	   	<select name="LanPort_1st" id="LanPort_1st">
              	 		<option value="4"><?echo I18N("h","Voice");?></option>
               			<option value="3"><?echo I18N("h","Video");?></option>
               			<option value="2"><?echo I18N("h","Best Effort");?></option>
	               		<option value="1"><?echo I18N("h","Background");?></option>
  	             	</select>
								</td>
							</tr>
							<tr>
								<td width="15%" align="right"><?echo I18N("h","Lan Port 2");?></td>
								<td width="3%" align="center">:</td>
								<td width="81%" align="left">
	               	<select name="LanPort_2nd" id="LanPort_2nd">
  	             		<option value="4"><?echo I18N("h","Voice");?></option>
    	           		<option value="3"><?echo I18N("h","Video");?></option>
      	         		<option value="2"><?echo I18N("h","Best Effort");?></option>
        	       		<option value="1"><?echo I18N("h","Background");?></option>
          	     	</select>
								</td>
							</tr>
							<tr>
								<td width="15%" align="right"><?echo I18N("h","Lan Port 3");?></td>
								<td width="3%" align="center">:</td>
								<td width="81%" align="left">
          	     	<select name="LanPort_3rd" id="LanPort_3rd">
            	   		<option value="4"><?echo I18N("h","Voice");?></option>
              	 		<option value="3"><?echo I18N("h","Video");?></option>
               			<option value="2"><?echo I18N("h","Best Effort");?></option>
               			<option value="1"><?echo I18N("h","Background");?></option>
	               	</select>
								</td>
							</tr>
							<tr>
								<td width="15%" align="right"><?echo I18N("h","Lan Port 4");?></td>
								<td width="3%" align="center">:</td>
								<td width="81%" align="left">
              	 	<select name="LanPort_4th" id="LanPort_4th">
	               		<option value="4"><?echo I18N("h","Voice");?></option>
  	             		<option value="3"><?echo I18N("h","Video");?></option>
    	           		<option value="2"><?echo I18N("h","Best Effort");?></option>
      	         		<option value="1"><?echo I18N("h","Background");?></option>
        	       	</select>
								</td>
							</tr>                                    
						</table>
					</div>
					<div id="qos_setup" style="display:none;">          
						<table width="100%" border="0" cellspacing="0" cellpadding="0" class="adv">                                    
							<tr>
								<td width="15%" align="right"><?echo I18N("h","Uplink Speed");?></td>
								<td width="3%" align="center">:</td>
								<td width="81%" align="left">
									<input id="upstream" type="text" size=6 maxlength=6>kbps <span>&nbsp;&lt;&lt;&nbsp;</span>
									<select id="select_upstream" modified="ignore" onchange="PAGE.OnChangeQOSUpstream();">
										<option value="0" selected><?echo I18N("h","Select Transmission Rate");?></option>
										<option value="128">128k</option>
										<option value="256">256k</option>
										<option value="384">384k</option>
										<option value="512">512k</option>
										<option value="1024">1M</option>
										<option value="2048">2M</option>
										<option value="3072">3M</option>
										<option value="5120">5M</option>
										<option value="10240">10M</option>
										<option value="20480">20M</option>
									</select>
									<?drawlabel("upstream");?>
								</td>
							</tr>
							<tr>
								<td width="15%" align="right"><?echo I18N("h","Downlink Speed");?></td>
								<td width="3%" align="center">:</td>
								<td width="81%" align="left">
									<input id="downstream" type="text" size=6 maxlength=6>kbps <span>&nbsp;&lt;&lt;&nbsp;</span>
									<select id="select_downstream" modified="ignore" onchange="PAGE.OnChangeQOSDownstream();">
										<option value="0" selected><?echo I18N("h","Select Transmission Rate");?></option>
										<option value="1024">1M</option>
										<option value="2048">2M</option>
										<option value="3072">3M</option>
										<option value="8192">8M</option>
										<option value="10240">10M</option>
										<option value="12288">12M</option>
										<option value="16384">16M</option>
										<option value="40960">40M</option>
										<option value="51200">50M</option>
										<option value="102400">100M</option>
									</select>
									<?drawlabel("downstream");?>
								</td>
							</tr>
							<tr>
								<td width="15%" align="right"><?echo I18N("h","Queue Type");?></td>
								<td width="3%" align="center">:</td>
								<td width="81%" align="left">
									<input id="Qtype_SPQ" name="qtype" type="radio" value="SPQ" onclick="PAGE.OnClickQtype(this.value);" /><?echo I18N("h","Strict Priority Queue");?>
									<input id="Qtype_WFQ" name="qtype" type="radio" value="WFQ" onclick="PAGE.OnClickQtype(this.value);" /><?echo I18N("h","Weighted Fair Queue");?>
								</td>
							</tr>
							<tr>            
							</tr>                        
						</table>
						<table id="queue_table" width="60%">
							<tr height="26">
								<th class="rc_gray_hd"><?echo I18N("h","Queue ID");?></th>
								<th class="rc_gray_hd" id="priority" ></th>
							</tr>
							<tr height="26">
								<td align="center"><?echo I18N("h","1");?></td>
								<td align="center" id="priority1"><input id="priority_dsc1" type="text" size="3" maxlength="3" />% <?drawlabel("priority_dsc1");?></td>
							</tr>
							<tr height="26">
								<td align="center"><?echo I18N("h","2");?></td>
								<td align="center" id="priority2"><input id="priority_dsc2" type="text" size="3" maxlength="3" />% <?drawlabel("priority_dsc2");?></td>
							</tr>
							<tr height="26">
								<td align="center"><?echo I18N("h","3");?></td>
								<td align="center" id="priority3"><input id="priority_dsc3" type="text" size="3" maxlength="3" />% <?drawlabel("priority_dsc3");?></td>
							</tr>
							<tr height="26">
								<td align="center"><?echo I18N("h","4");?></td>
								<td align="center" id="priority4"><input id="priority_dsc4" type="text" size="3" maxlength="3" />% <?drawlabel("priority_dsc4");?></td>
							</tr>
						</table>
					</div>
				</div>
				<div id="qos_setup_rules" style="display:none;">
					<div>
						<table id="qos_table" width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
						<tr>
							<td colspan="4" class="gray_bg gray_border_btm"><p class="subitem"><?=$QOS_MAX_COUNT?> -- <?echo I18N("h","Classification Rules");?></p>
								<p><?echo I18N("h","Remaining number of rules that can be created");?>: <span id="rmd" style="color:red;"></span>                        
							</td>
						</tr>                    
							<col width="10px"></col>
							<col width="70px"></col>
							<col width="70px"></col>
							<col width="70px"></col>
<?
$INDEX = 1;
while ($INDEX <= $QOS_MAX_COUNT)	{dophp("load", "/htdocs/webinc/body/adv_qos_list.php");	$INDEX++;}
?>
						</table>
					</div>
				</div>
                                    
			</div>
			<table width="100%">
				<tr>
					<td class="rc_gray5_ft">
						<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
						<button value="submit" class="submitBtn floatRight" onclick="BODY.OnSubmit();"><b><?echo I18N("h","Save");?></b></button>
					</td>
				</tr>
			</table>
  	</div>
	</div>
</form>
