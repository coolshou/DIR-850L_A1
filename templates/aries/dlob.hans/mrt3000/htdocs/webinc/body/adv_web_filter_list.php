<? include "/htdocs/webinc/body/draw_elements.php"; ?>
		<tr>
			<td nowrap="nowrap" class="gray_bg border_left">
				<input type="hidden" id="uid_<?=$INDEX?>" value="">
				<input type="hidden" id="description_<?=$INDEX?>" value="">
				<input type="checkbox"id="en_<?=$INDEX?>"  />
			</td>
			<td nowrap="nowrap" class="gray_bg border_left">
            	<input type=text id="url_<?=$INDEX?>" size=44 maxlength=99>
			</td>
			<?
			if ($FEATURE_NOSCH!="1")
			{
				echo '<td class="centered">\n';
				DRAW_select_sch("sch_".$INDEX, I18N("h","Always"), "-1", "", "0", "narrow");
				echo '&nbsp;<input type="button"  id="sch_'.$INDEX.'_btn" value="'.I18N("h","New Schedule").'"'.
					' onclick="javascript:self.location.href=\'tools_sch.php\'">\n';
				echo '</td>\n';
			}
			?>
		</tr>
