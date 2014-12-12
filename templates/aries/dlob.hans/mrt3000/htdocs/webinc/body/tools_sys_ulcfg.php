<div id="content" class="maincolumn">
<?
include "/htdocs/webinc/body/draw_elements.php";

if ($_GET["RESULT"]=="SUCCESS")
{
	$f_style = ' style="display:none;"';
}
else
{
	$f_style = ' style="display:block;"';
}
?><div class="rc_gradient_bd h_initial" <?=$f_style?>>
	<h2><?echo I18N("h","Restore Invalid");?></h2>
	<h6><?echo I18N("h","The restored configuration file is incorrect.")." ".
		I18N("h","The restored file may not be intended for this device, may be from an incompatible version of this product, or may be corrupted.");?><br />
	<?echo I18N("h","Try the restore again with valid restore configuration file.");?><br />
	<?echo I18N("h","Please press the button below to continue configuring the router.");?><h6>
	<br /><input type="button" value="<?echo I18N("h","Continue");?>" onclick="history.back();" />
</div>
</div>
