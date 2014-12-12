<form id="mainform" onsubmit="PAGE.OnClick_Run();return false;">
<div class="orangebox">
	<h1><?echo i18n("IPV6 Routing");?></h1>
	<p>
		<?echo i18n('This page displays IPV6 routing details configured for your router.');?>
	</p>
</div>
<div class="blackbox">
	<h2><?echo i18n("IPV6 Routing Table");?></h2>
     <table id="routing_list" class="general" >
                <tr>
                        <th width=40%><?echo i18n("Destination IP");?></th>
                        <th width=30%><?echo i18n("Gateway");?></th>
                        <th width=15%><?echo i18n("Metric");?></th>
                        <th width=15%><?echo i18n("Interface");?></th>
                </tr>
       </table>
<div class="emptyline"></div>
<div class="emptyline"></div>
<div class="emptyline"></div>
<div class="emptyline"></div>
<div class="emptyline"></div>
</div>
</form>
