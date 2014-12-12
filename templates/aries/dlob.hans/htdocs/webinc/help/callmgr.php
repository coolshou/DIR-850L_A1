<strong><?echo i18n("Helpful Hints");?>...</strong>
<br/><br/>
<p>&nbsp;&#149;&nbsp;&nbsp;<?

	echo "<strong>".i18n("Caller ID Display").":</strong><br/>";
    	echo i18n("Allows you to see the Callers ID when they make a call to you.");

?></p>

<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo "<strong>".i18n("Caller ID Delivery").":</strong><br/>";
    	echo i18n("When Enabled this will block your Outgoing Caller ID so that when you make a call it will make you appear to be an anonymous caller.");
?></p>

<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo "<strong>".i18n("Call Waiting").":</strong><br/>";
    	echo i18n("When Eabled this will allow you to answer another incoming call when you have already one call in progress.");
?></p>

<p>&nbsp;&#149;&nbsp;&nbsp;<?
	echo "<strong>".i18n("Call Forwarding").":</strong><br/>";
    	echo i18n("Unconditional - Allows you to Enable or Disable the Call Forwarding Unconditional feature. When Enabled it will relay all calls to a different specified number.");
?></p>
<p><?
	echo i18n("Busy - Allows you to Enable or Disable the Call Forwarding Busy feature. When Enabled it will relay all calls when you are already on another call to a different specified number.");
?></p>
<p><?
    	echo i18n("No Answer - Allows you to Enable or Disable the Call Forwarding No Answer feature. When Enabled it will relay all calls when you do not answer the call within specified time to a different specified number.");
?></p>
<p><?
    	echo i18n("No Answer Timer - This will be the number that calls you do not answer the phone are to be forwarded to when No Answer is Enabled.");
?></p>
<p><?
    	echo i18n("Not reachable - Allows you to Enable or Disable the Call Forwarding Not reachable feature. When Enabled it will relay all calls when you are not reachable to a different specified number.");

?></p>
<p><?
	echo "<strong>".i18n("Hotkey")."</strong><br/>";
	echo i18n("*50*666 - Allows you to Disable the Caller ID Delivery.<br/>");
	echo i18n("*51*666 - Allows you to Enable the Caller ID Delivery.<br/>");
	echo i18n("*40*666 - Allows you to Disable the Call Waiting.<br/>");
	echo i18n("*41*666 - Allows you to Enable the Call Waiting.<br/>");
	echo i18n("*30*666 - Allows you to Disable the Call Forwarding.<br/>");
	echo i18n("*31*666*&lt;phone number&gt; - Allows you to Set the Call Forwarding Unconditional.<br/>");
	echo i18n("*32*666*&lt;phone number&gt;*&lt;sec&gt; - Allows you to Set the Call Forwarding No Answer.<br/>");
	echo i18n("*33*666*&lt;phone number&gt; - Allows you to Set the Call Forwarding Busy.<br/>");
	echo i18n("*34*666*&lt;phone number&gt; - Allows you to Set the Call Forwarding Not Reachable.<br/>");
	echo i18n("*99* - Allows you to Save changes.");
?></p>
