<strong><?echo i18n("Helpful Hints");?>...</strong>
<br/><br/>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo i18n("Give each rule a <strong>Name</strong> that is meaningful to you.");	
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo i18n("Each rule can either <strong>Allow</strong> or <strong>Deny</strong> access from the WAN.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo I18N("h",'Up to eight ranges of WAN IP addresses can be controlled by each rule. The checkbox by each IP range can be used to disable ranges already defined.');
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo I18N("h",'The starting and ending IP addresses are WAN-side address.');
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo i18n("Click the <strong>Add</strong> button to store a finished rule in the Rules List below.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo i18n("Click the <strong>Edit</strong> icon in the Rules List to change a rule.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo i18n("Click the <strong>Delete</strong> icon in the Rules List to permanently remove a rule.");
?></p>
<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo '<a href="/spt_adv.php#InboundFilter">'.I18N("h","More...").'</a>';
?></p>
