	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<div class="rc_gradient_hd"><h2><?echo I18N("h","Share your files and devices");?></h2></div>
		<div class="rc_gradient_bd h_parental">
			<h4><?echo I18N("h","Setup miiiCasa");?></h4>
			<div class="gradient_form_content">
				<p><?echo I18N("h","You can enable or disable miiiCasa service, once you disable miiiCasa service from Router Setting, no one can use miiiCasa service through this router.");?></p>
				<p class="assistant"><?echo I18N("h","miiiCasa Settings");?></p>
				<p class="dent_i">
					<input type="radio" name="miiicasa_active" id="dis_miiicasa" value="0" />
					<label for="dis_miiicasa"><?echo I18N("h","Disable");?></label>
				</p>
				<p class="dent_i">
					<input type="radio" name="miiicasa_active" id="en_miiicasa" value="1" />
					<label for="en_miiicasa"><?echo I18N("h","Enable");?></label>
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
