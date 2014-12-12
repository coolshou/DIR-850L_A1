<strong><?echo i18n("Helpful Hints");?>...</strong>
<br/><br/>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo I18N("h","Use this feature if you are trying to execute one of the listed network applications and it is not communicating as expected.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo i18n("Use the <strong>Application Name</strong> drop-down menu to view a list of pre-defined applications that you can select from. If you select one of the pre-defined applications, click the arrow button next to the drop-down menu to fill out the appropriate fields.");
?></p>
<p<?if ($FEATURE_NOSCH=="1")echo ' style="display:none"';?>>&nbsp;&#149;&nbsp;&nbsp;<?
	echo i18n("Select a schedule for when the service will be enabled. If you do not see the schedule you need in the list of schedules, go to the <a href='/tools_sch.php'>Tools -> Schedules</a> screen and create a new schedule.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo '<a href="/spt_adv.php#App">'.I18N("h","More...").'</a>';
?></p>
