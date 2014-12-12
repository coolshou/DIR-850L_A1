
<html>
	<head>
		<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
		<title>D-LINK SYSTEMS, INC. | Web File Access : Login</title>
    	<link rel="stylesheet" rev="stylesheet" href="css/style.css" tppabs="/webaccess/css/style.css" type="text/css" />    	
		<script type="text/javascript" src="fancybox/json2.js" tppabs="/webaccess/fancybox/json2.js"></script>
    	<script language="JavaScript" src="js/object.js" tppabs="/webaccess/js/object.js"></script>
		<script language="JavaScript" src="js/xml.js" tppabs="/webaccess/js/xml.js"></script>
		<script language="JavaScript" src="js/public.js" tppabs="/webaccess/js/public.js"></script>
		<script language="JavaScript" src="js/md5.js" tppabs="/webaccess/js/md5.js"></script>
		<script language="JavaScript">
			var my_xml;
			var media_info;
		
			load_lang_obj();	// you have to load language object for displaying words in each html page and load html object for the redirect or return page
			document.onkeypress = login_key_handler;  //click the "Enter" Key can also browse web gui
	
			function get_settings_xml(http_req){	
				my_xml = http_req.responseXML;
				get_by_id("fw_ver").innerHTML = get_node_value(my_xml, "fw_ver");
				get_by_id("hw_ver").innerHTML = get_node_value(my_xml, "hw_ver");				
	
				disable_all_btn(false);
			}
			
			function show_auth_fail(){
					//msg_obj.warning_msg('CMSG003');
					//alert("<? echo I18N("h", "Fail");?>");
					alert("<? echo I18N("h", "User Name or Password is incorrect.");?>");
					disable_all_btn(false);
			}
			
			function redirect_category_page(http_req){
				var my_txt = http_req.responseText;		
				var resp_info;
				
				try {
					resp_info = JSON.parse(my_txt);
				} catch(e) {
					show_auth_fail();
					return;
				}
				if (resp_info.status == "ok" && resp_info.error == null){				
					location.href = "category_view.php"/*tpa=/webaccess/category_view.php*/;
				}else if (resp_info.errno == 5003){
					show_auth_fail();		
			}
			}
			
			function send_request(){
				var xml_request = new XMLRequest(redirect_category_page);		
				var user_name = (get_by_id("user_name").value).toLowerCase();	// always make user name to lowercase
				var user_pwd = get_by_id("user_pwd").value;				
				var digest;
				var para;
				digest = hex_hmac_md5(user_pwd, user_name + media_info.challenge);				
				para = "id=" + user_name + "&password=" + digest;				
				
				xml_request.exec_auth_cgi(para);
			}
			
			function get_auth_info_result(http_req){
				var my_txt = http_req.responseText;
				try {
					media_info = JSON.parse(my_txt);
				} catch(e) {
					show_auth_fail();
					return;
				}	
					document.cookie	= "uid=" + media_info.uid+";path=/";
					send_request();
				}
			
			function get_auth_info(){
				var xml_request = new XMLRequest(get_auth_info_result);
				
				xml_request.exec_auth_cgi();
			}
			
			function login_key_handler(e){
				var which_key;
				
				if (document.all) {
					which_key = window.event.keyCode;
				}else{
					which_key = e.which;
				}

				if (which_key == 13){ //click the "Enter" Key can also browse web gui
					get_auth_info();
                                }
			}

			function get_system_info_result(http_req){	
				my_xml = http_req.responseXML;
				get_by_id("fw_ver").innerHTML = get_node_value(my_xml, "fw_ver");
				get_by_id("hw_ver").innerHTML = get_node_value(my_xml, "hw_ver");
				get_by_id("model").innerHTML = get_node_value(my_xml, "model");			
			}

			function get_system_info(){
				var xml_request = new XMLRequest(get_system_info_result);
				var para = "request=get_system_info";
							
				xml_request.exec_webfile_cgi(para);
			}
   	</script>
	</head>
	<!--body onLoad="disable_all_btn(false);get_by_id('user_name').focus();get_login_info('no_auth','fw_ver', 'hw_ver')"-->  
	<body onLoad="get_system_info()">
   	<div id="outside_1col">
      	<table id="table_shell" cellspacing="0" align="center">
         	<tr>
            	<td>
               	<div id="header_container">
                  	<div id="info_bar">
                     	<div class="fwv"><? echo I18N("h", "Firmware Version");?> : <? echo query("/runtime/device/firmwareversion"); ?><span id="fw_ver" align="left"></span></div>
                     	<div class="hwv"><? echo I18N("h", "Hardware Version");?> : <? echo query("/runtime/device/hardwareversion"); ?><span id="hw_ver" align="left"></span></div>
                      	<div class="pp"><? echo I18N("h", "Product Page");?> : <? echo query("/runtime/device/modelname"); ?><a href="javascript:check_is_modified('http://support.dlink.com/')"><span id="model" align="left"></span></a></div>
                    	</div>
             		</div>
                	<table id="masthead_container" border="0" cellspacing="0">
                 		<tr>
                    		<td align="center" valign="middle"><img src="webfile_images/wlan_masthead.gif" tppabs="/webaccess/webfile_images/wlan_masthead.gif" width="836" height="92"></td>
                   	</tr>
               	</table>
                	<table id="content_container" border="0" cellspacing="0">
                  	<tr>
                     	<td id="sidenav_container">&nbsp;</td>
                    		<td id="maincontent_container">                      		
                      		<div id="maincontent_1col">
                      			<div class="section">
                						<div class="section_head">
                    						<h2><? echo I18N("h", "WEB FILE ACCESS LOGIN");?></h2>
                    						<p><? echo I18N("h", "Log in to the web file access server");?> :</p>
                      					<table border="0" cellpadding="2" cellspacing="2" width="100%">	
		                              	<tr height="24">
	                                    	<td width="40%" align="right"><b><? echo I18N("h", "User Name");?>&nbsp;&nbsp;:&nbsp;&nbsp;</b></td>
	                                    	<td width="60%">												
                                    			<input type="text" id="user_name" name="user_name" size="20" maxlength="15">
			                            		</td>
			                            	</tr>
			                              <tr height="24">
	                                    	<td width="40%" align="right"><b><? echo I18N("h", "Password");?>&nbsp;&nbsp;:&nbsp;&nbsp;</b></td>
	                                    	<td width="60%">	                                    		
														<input type="password" id="user_pwd" name="user_pwd" size="20" maxlength="15" />
														<input type="button" id="login" name="login" class="button_submit_padleft" value="<? echo I18N("h", "Login");?>" onClick="get_auth_info()" />
														</td>
													</tr>
												</table>		                                																									      	
                            		</div>
            						</div>
            					</div>														
                    		</td>
                  		<td id="sidehelp_container">&nbsp;</td>
             			</tr>
         			</table>
         			<table id="footer_container" border="0" cellspacing="0">
                  	<tr>
                    		<td><img src="webfile_images/wireless_tail.gif" tppabs="/webaccess/webfile_images/wireless_tail.gif" width="114" height="35" /></td>
                     	<td>&nbsp;</td>
               		</tr>
              		</table>
   				</td>
            </tr>            
        	</table>
      	<div id="copyright">Copyright &copy; 2012 D-Link Corporation. All rights reserved.</div>
    	</div>
	</body>
</html>
