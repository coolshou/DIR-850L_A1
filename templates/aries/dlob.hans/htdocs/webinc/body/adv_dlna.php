<!-- css for calling explorer.php -->
<link rel="stylesheet" href="/portal/comm/smbb.css" type="text/css"> 
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("DLNA Settings");?></h1>
	<p><?echo i18n("DLNA (Digital Living Network Alliance) is the standard for the interoperability of Network Media Devices (NMDs). The user can enjoy multi-media applications (music, pictures and videos) on your network connected PC or media devices.");?>	
	</p>
	<p>
		<input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" />
		<input type="button" name="But_Refresh" value="<?echo i18n("Refresh");?>" onclick="PAGE.refresh();">
	</p>
</div>
<div class="blackbox">
	<h2><?echo i18n("Media Sever Settings");?></h2>		
	<table border="0" cellpadding="0" cellspacing="0" style="border-collapse: collapse" bordercolor="#111111" id="AutoNumber1" height="74" width="500">
        <tr>                            
          <td height="32">&nbsp;</td>
          <td height="32">
          	<input type="checkbox" id="dms_active" onClick="PAGE.check_dms_enable(1)" style="margin-top:15px;">&nbsp;
          </td>	
          <td height="32">          	
          	<strong><?echo i18n("Share media libraries with devices");?></strong>
          </td>
		</tr>				
		<tr>
         <td height="32" colspan=2>&nbsp;</td>         
         <td height="32">
         	 <?echo i18n("If you agree to share media with devices, any computer or device that connects to your network can play your shared music, pictures and videos.");?><br><br>					
          </td>
		</tr>				
		<tr>
		  <td height="32" colspan=2>&nbsp;</td>			  
          <td height="32">         	
          <?echo i18n("NOTE: The shared media may not be secure. Allowing any devices to stream is recommended only on secure networks.");?><br><br>	        	
          </td>
		</tr>
	</table>			 
	<table border="0" cellpadding="0" cellspacing="0" style="border-collapse: collapse" bordercolor="#111111" id="AutoNumber1" height="74" width="500">			
		<tr>
          <td width=20 height="32">&nbsp;</td> 
          <td height="32"><strong><?echo i18n("Name your media library");?>:</strong></td>
          <td height="32">                  	
			<input type=text id="dms_name" size=20 value="" maxlength=15>
          </td>
		</tr>
        <tr>
          <td height="32">&nbsp;</td> 		         
          <td height="32"><strong><?echo i18n("Folder");?>:</strong></td>
          <td height="32">  
          <input type="checkbox" id="dms_root" value="ON" onClick="PAGE.check_path(1)"><?echo i18n("root");?>
          </td>
		</tr>
        <tr id="chamber2" style="display">
          <td height="32" colspan=2>&nbsp;</td>
          <td height="29">
            <input type=text id="the_sharepath" size=30 readonly>
            <input type="button" id="But_Browse" value=<?echo i18n("Browse");?> onClick="PAGE.open_browser();">
            <input type="hidden" id="f_flow_value" size="20">
            <input type="hidden" id="f_device_read_write" size="2">
            <input type="hidden" id="f_read_write" size="2">
		  </td>
        </tr>
	</table>
	<div class="gap"></div>
</div>
<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
</form>
