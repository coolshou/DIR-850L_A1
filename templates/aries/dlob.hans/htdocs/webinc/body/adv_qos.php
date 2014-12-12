<? 
include "/htdocs/webinc/body/draw_elements.php";
include "/htdocs/phplib/xnode.php";
?>
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("QoS SETTINGS");?></h1>
	<p>
		<?
		if ($FEARURE_UNCONFIGURABLEQOS != "1")
		{
		echo i18n("Use this section to configure D-Link's QoS Engine powered by QoS Engine Technology. This QoS Engine improves your online gaming experience by ensuring that your game traffic is prioritized over other network traffic, such as FTP or Web.For best performance, use the Automatic Classification option to automatically set the priority for your applications.");
		}
		else
		{
		echo i18n("Use this section to configure D-Link's Smart QoS. ");
	    echo i18n("The QoS Engine improves your online gaming experience by ensuring that your game traffic is prioritized over other network traffic, such as FTP or Web. ");
		}
		?>
	</p>
	<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</div>
<!--
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h2><?echo i18n("QoS SETTINGS");?></h2>
	<p><?echo i18n("QoS rules can be used to control traffic.");?></p>
	<p>	<input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</div>
-->

<div class="blackbox">
	<h2><?echo i18n("QoS Setup");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable QoS");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input type="checkbox" id="en_qos" onclick="PAGE.OnClickQOSEnable();" /></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Uplink Speed");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="upstream" type="text" size=6 maxlength=7>
			kbps <span>&nbsp;&lt;&lt;&nbsp;</span>
			<select id="select_upstream" modified="ignore" onchange="PAGE.OnChangeQOSUpstream();">
				<option value="0" selected><?echo i18n("Select Transmission Rate");?></option>
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
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Downlink Speed");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="downstream" type="text" size=6 maxlength=7>
			kbps <span>&nbsp;&lt;&lt;&nbsp;</span>
			<select id="select_downstream" modified="ignore" onchange="PAGE.OnChangeQOSDownstream();">
				<option value="0" selected><?echo i18n("Select Transmission Rate");?></option>
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
		</span>
	</div>
	<div class="textinput" <?if ($FEARURE_UNCONFIGURABLEQOS == "1") echo ' style="display:none;"';?>>
		<span class="name"><?echo i18n("Queue Type");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="Qtype_SPQ" name="qtype" type="radio" value="SPQ" onclick="PAGE.OnClickQtype(this.value);" /><?echo i18n("Strict Priority Queue");?>
			<input id="Qtype_WFQ" name="qtype" type="radio" value="WFQ" onclick="PAGE.OnClickQtype(this.value);" /><?echo i18n("Weighted Fair Queue");?>
		</span>
	</div>
	<div class="gap"></div>
	<table id="queue_table" class="general" <?if ($FEARURE_UNCONFIGURABLEQOS == "1") echo ' style="display:none;"';?>>
		<tr height="26">
			<th><?echo i18n("Queue ID");?></th>
			<th id="priority" ></th>
		</tr>
		<tr height="26">
			<td class="centered"><?echo i18n("1");?></td>
			<td class="centered" id="priority1"><input id="priority_dsc1" type="text" size="3" maxlength="3" />%</td>
		</tr>			
		<tr height="26">
			<td class="centered"><?echo i18n("2");?></td>
			<td class="centered" id="priority2"><input id="priority_dsc2" type="text" size="3" maxlength="3" />%</td>
		</tr>	
		<tr height="26">
			<td class="centered"><?echo i18n("3");?></td>
			<td class="centered" id="priority3"><input id="priority_dsc3" type="text" size="3" maxlength="3" />%</td>
		</tr>
		<tr height="26">
			<td class="centered"><?echo i18n("4");?></td>
			<td class="centered" id="priority4"><input id="priority_dsc4" type="text" size="3" maxlength="3" />%</td>
		</tr>
	</table>
	<div class="gap"></div>
</div>

<div class="blackbox" <?if ($FEARURE_UNCONFIGURABLEQOS == "1") echo ' style="display:none;"';?>>
	<h2><?=$QOS_MAX_COUNT?> -- <?echo i18n("Classification Rules");?></h2>
	<p><?echo i18n("Remaining number of rules that can be created");?>: <span id="rmd" style="color:red;"></span></p>
	<table id="qos_table" class="general">
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

<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>

</form>

