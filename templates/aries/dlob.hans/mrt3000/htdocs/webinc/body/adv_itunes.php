	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<div class="rc_gradient_hd"><h2><?echo I18N("h","Share your files and devices");?></h2></div>
		<div class="rc_gradient_bd h_parental">
			<h4><?echo I18N("h","iTunes Server");?></h4>
			<div class="gradient_form_content">
				<p><?echo I18N("h","You can share video/audio/image files stored on the hard disk drive.");?></p>
				<p class="assistant"><?echo I18N("h","iTunes Server");?></p>
				<p class="dent_i">
					<input type="radio" name="itunes_active" id="inactive" value="0" onclick="PAGE.check_itunes_enable(1);" />
					<label for="inactive"><?echo I18N("h","Disable");?></label>
				</p>
				<p class="dent_i">
					<input type="radio" name="itunes_active" id="active" value="1" onclick="PAGE.check_itunes_enable(1);" />
					<label for="active"><?echo I18N("h","Enable");?></label>
					<span class="dent_i" style="display:none;">
						<input type="checkbox" id="itunes_root" value="ON" onClick="PAGE.check_path(1);" />
						<label for="itunes_root"><?echo I18N("h","Start from root folder");?></label><br />
						<span id="chamber2">
							<input type=text id="the_sharepath" size=30 readonly />
							<input type="button" id="But_Browse" value="<?echo I18N("h","Browse");?>" onClick="PAGE.open_browser();" />
							<input type="hidden" id="f_flow_value" size="20" />
							<input type="hidden" id="f_device_read_write" size="2" />
							<input type="hidden" id="f_read_write" size="2" />
						</span>
					</span>
					<span id="select_partition"></span>	
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
