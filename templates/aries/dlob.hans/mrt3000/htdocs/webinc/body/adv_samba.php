	<?
	function get_br0ip()
	{
		$phyp = XNODE_getpathbytarget("/runtime", "phyinf", "name", "br0", 0);
		if ($phyp!="" && query($phyp."/valid")=="1"){$phyuid = query($phyp."/uid");}
		$infp = XNODE_getpathbytarget("/runtime", "inf", "phyinf", $phyuid, 0);
		if ($infp!=""){$ipaddr = query($infp."/inet/ipv4/ipaddr");}
		return $ipaddr;
	}
	
	$host_name = query("/device/hostname");
	$lan_ip = get_br0ip();
	$p = XNODE_getpathbytarget("device/account", "entry", "name", "admin", 0);
	$samba_account = query($p."/name");
	$samba_passwd = query($p."/password");
	?>
	
	
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<div class="rc_gradient_hd"><h2><?echo I18N("h","Share your files and devices");?></h2></div>
		<div class="rc_gradient_bd h_parental">
			<h4><?echo I18N("h","Samba");?></h4>
			<div class="gradient_form_content">
				<p><?echo I18N("",'When you enable Samba Server, you can enter <strong>"\\\\$1"</strong> or <strong>"\\\\$2"</strong> into the location bar of the windows file explorer to access your contents in this device and share video/audio/image files stored on the hard disk drive. Samba ID: <strong>"$3"</strong>, Samba Password: <strong>"$4"</strong>.', $host_name, $lan_ip, $samba_account, $samba_passwd);?></p>
            	<p><font size=1><b>
					<ul>
						<li><?echo I18N("h","Your Samba ID and password is the same as your admin name / password.");?></li>
						<li><?echo I18N("h","If your admin password contains the '$' sign, your Samba server will not work properly.");?></li>
					</ul>
				</b></font></p>
				<p class="assistant"><?echo I18N("h","Samba Server");?></p>
				<p class="dent_i">
					<input type="radio" name="samba_active" id="dis_samba" value="0" />
					<label for="dis_samba"><?echo I18N("h","Disable");?></label>
				</p>
				<p class="dent_i">
					<input type="radio" name="samba_active" id="en_samba" value="1" />
					<label for="en_samba"><?echo I18N("h","Enable");?></label>
				</p>
			</div>
		</div>
		<div class="rc_gradient_submit">
			<button type="button" class="submitBtn floatLeft" onclick="self.location.href='./share.php';">
				<b><?echo I18N("h","Cancel");?></b>
			</button>
			<button type="button" class="submitBtn floatRight" onclick="BODY.OnSubmit();">
				<b><?echo I18N("h","Save & Exit");?></b>
			</button><i></i>
		</div>
	</div>
	</form>
