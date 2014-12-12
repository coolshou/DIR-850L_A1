<form id="mainform" onsubmit="PAGE.OnClick_Run();return false;">
<div class="orangebox">
	<h1><?echo i18n("Routing");?></h1>
	<p>
		<b><?echo i18n('Routing Table');?></b>
		<p>
		<?echo i18n('This page displays the routing details configured for your router.');?>
	</p>
</div>
<div class="blackbox">
	<h2><?echo i18n("Routing Table");?></h2>
     <table id="routing_list" class="general" >
                <tr>
                        <th width=13%><?echo I18N("h","Destination");?></th>
                        <th width=10%><?echo I18N("h","Gateway");?></th>
                        <th width=13%><?echo I18N("h","Genmask");?></th>
                        <th width=8%><?echo I18N("h","Metric");?></th>
                        <th width=8%><?echo I18N("h","Iface");?></th>
                        <th width=9%><?echo I18N("h","Creator");?></th>
                </tr>
       </table>
<div class="emptyline"></div>
<div class="emptyline"></div>
<div class="emptyline"></div>
<div class="emptyline"></div>
<div class="emptyline"></div>
</div>
</form>