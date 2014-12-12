<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<title>Untitled Document</title>
	<script type="text/javascript" src="./js/roundcorner/jquery-1.4.2.min.js"></script>
	<script type="text/javascript" src="./js/menu/ddaccordion.js">
	/***********************************************
	* Accordion Content script- (c) Dynamic Drive DHTML code library (www.dynamicdrive.com)
	* Visit http://www.dynamicDrive.com for hundreds of DHTML scripts
	* This notice must stay intact for legal use
	***********************************************/
	</script>
	<script type="text/javascript">
	ddaccordion.init({ //top level headers initialization
		headerclass: "expandable", //Shared CSS class name of headers group that are expandable
		contentclass: "categoryitems", //Shared CSS class name of contents group
		revealtype: "click", //Reveal content when user clicks or onmouseover the header? Valid value: "click", "clickgo", or "mouseover"
		mouseoverdelay: 200, //if revealtype="mouseover", set delay in milliseconds before header expands onMouseover
		collapseprev: true, //Collapse previous content (so only one open at any time)? true/false 
		defaultexpanded: [0], //index of content(s) open by default [index1, index2, etc]. [] denotes no content
		onemustopen: false, //Specify whether at least one header should be open always (so never all headers closed)
		animatedefault: false, //Should contents open by default be animated into view?
		persiststate: true, //persist state of opened contents within browser session?
		toggleclass: ["", "openheader"], //Two CSS classes to be applied to the header when it's collapsed and expanded, respectively ["class1", "class2"]
		togglehtml: ["prefix", "", ""], //Additional HTML added to the header when it's collapsed and expanded, respectively  ["position", "html1", "html2"] (see docs)
		animatespeed: "fast", //speed of animation: integer in milliseconds (ie: 200), or keywords "fast", "normal", or "slow"
		oninit:function(headers, expandedindices){ //custom code to run when headers have initalized
		//do nothing
		},
		onopenclose:function(header, index, state, isuseractivated){ //custom code to run whenever a header is opened or closed
		//do nothing
		}
	})

	ddaccordion.init({ //2nd level headers initialization
		headerclass: "subexpandable", //Shared CSS class name of sub headers group that are expandable
		contentclass: "subcategoryitems", //Shared CSS class name of sub contents group
		revealtype: "click", //Reveal content when user clicks or onmouseover the header? Valid value: "click" or "mouseover
		mouseoverdelay: 200, //if revealtype="mouseover", set delay in milliseconds before header expands onMouseover
		collapseprev: true, //Collapse previous content (so only one open at any time)? true/false 
		defaultexpanded: [], //index of content(s) open by default [index1, index2, etc]. [] denotes no content
		onemustopen: false, //Specify whether at least one header should be open always (so never all headers closed)
		animatedefault: false, //Should contents open by default be animated into view?
		persiststate: true, //persist state of opened contents within browser session?
		toggleclass: ["opensubheader", "closedsubheader"], //Two CSS classes to be applied to the header when it's collapsed and expanded, respectively ["class1", "class2"]
		togglehtml: ["none", "", ""], //Additional HTML added to the header when it's collapsed and expanded, respectively  ["position", "html1", "html2"] (see docs)
		animatespeed: "fast", //speed of animation: integer in milliseconds (ie: 200), or keywords "fast", "normal", or "slow"
		oninit:function(headers, expandedindices){ //custom code to run when headers have initalized
		//do nothing
		},
		onopenclose:function(header, index, state, isuseractivated){ //custom code to run whenever a header is opened or closed
		//do nothing
		}
	})

	ddaccordion.init({ //2nd level headers initialization
		headerclass: "subexpandable_end", //Shared CSS class name of sub headers group that are expandable
		contentclass: "subcategoryitems_end", //Shared CSS class name of sub contents group
		revealtype: "click", //Reveal content when user clicks or onmouseover the header? Valid value: "click" or "mouseover
		mouseoverdelay: 200, //if revealtype="mouseover", set delay in milliseconds before header expands onMouseover
		collapseprev: true, //Collapse previous content (so only one open at any time)? true/false 
		defaultexpanded: [], //index of content(s) open by default [index1, index2, etc]. [] denotes no content
		onemustopen: false, //Specify whether at least one header should be open always (so never all headers closed)
		animatedefault: false, //Should contents open by default be animated into view?
		persiststate: true, //persist state of opened contents within browser session?
		toggleclass: ["opensubheader_end", "closedsubheader_end"], //Two CSS classes to be applied to the header when it's collapsed and expanded, respectively ["class1", "class2"]
		togglehtml: ["none", "", ""], //Additional HTML added to the header when it's collapsed and expanded, respectively  ["position", "html1", "html2"] (see docs)
		animatespeed: "fast", //speed of animation: integer in milliseconds (ie: 200), or keywords "fast", "normal", or "slow"
		oninit:function(headers, expandedindices){ //custom code to run when headers have initalized
		//do nothing
		},
		onopenclose:function(header, index, state, isuseractivated){ //custom code to run whenever a header is opened or closed
		//do nothing
		}
	})
	</script>
	<style type="text/css">
		body { margin:0; padding:0; font-family:Arial, Helvetica, sans-serif;}
		ol { margin:18px; padding:0; list-style:none;}
		a {outline: none;}
		.maincolumn_content , .arrowlistmenu { border-color:#cbcbcb #eaeaea #eaeaea #cbcbcb; border-width:1px; border-style:solid;}
		.maincolumn_content { padding:0; height:460px; width:520px; margin:0 0 0 10px; float:left; line-height:1.2em; overflow:hidden;}
		.arrowlistmenu { padding:10px; height:440px; width:200px; margin:0; float:left; font-size:13px; color:#272727; line-height:1.5em; overflow:auto;}
		.arrowlistmenu h3 { font-size:13px; margin:0; padding:0 0 0 20px; cursor:pointer; background-repeat:no-repeat;}
		.menuheader_start { background-image:url(../pic/tree_menu_start.gif);}
		.menuheader_mid { background-image:url(../pic/tree_menu_mid.gif);}
		.menuheader_end { background-image:url(../pic/tree_menu_end.gif);}
		.openheader{ background-position:-20px -30px;}
		.arrowlistmenu ul { margin:0; padding:0 0 0 20px; list-style:none;}
		.arrowlistmenu ul li { margin:0; padding:0;}
		.arrowlistmenu ul li.dive { background:url(../pic/tree_menu_dive.gif) 0 0 no-repeat;}
		.arrowlistmenu ul li.dive_end { background:url(../pic/tree_menu_dive_end.gif) -20px -30px no-repeat;}
		.arrowlistmenu ul li a { display:block; padding:2px 8px; margin-left:17px; color:#272727; text-decoration:none;}
		.arrowlistmenu ul li a:hover { text-decoration:underline; color:#000; background-color:#ececec; opacity: 0.6; -ms-filter:alpha(opacity=60);}
		.arrowlistmenu ul li a.subexpandable { padding:2px 5px 2px 20px; margin:0;}
		.arrowlistmenu ul li a.opensubheader { background:url(../pic/tree_menu_sub_oc.gif) no-repeat;}
		.arrowlistmenu ul li a.closedsubheader { background:url(../pic/tree_menu_sub_oc.gif) -20px -30px no-repeat;}
		.arrowlistmenu ul li a.subexpandable_end { padding:2px 5px 2px 20px; margin:0;}
		.arrowlistmenu ul li a.opensubheader_end { background:url(../pic/tree_menu_end.gif) no-repeat;}
		.arrowlistmenu ul li a.closedsubheader_end { background:url(../pic/tree_menu_end.gif) -20px -30px no-repeat;}
		.arrowlistmenu ul .menu_selected { background-color:#ececec; padding:2px 8px; margin-left:17px; display:block;}
		.arrowlistmenu ul .subcategoryitems { background:url(../pic/tree_menu_stem.gif) repeat-y;}
		.categoryitems { background: url(../pic/tree_menu_stem.gif) repeat-y;}
		ul.stem_end { background:none;}
	</style>
</head>
<body>
<ol id="show_info">
	<!-- left Menu -->
	<li class="arrowlistmenu">
		<h3 class="menuheader_start expandable"><?echo I18N("h","Network");?></h3>
		<ul class="categoryitems">
			<li class="dive"><a href="javascript:jumpto('/help/help_net_wan.php')" class="menu_selected"><?echo I18N("h","Internet");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_net_lan.php')"><?echo I18N("h","Network Settings");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_net_advance.php')"><?echo I18N("h","Advanced Network");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_net_virtual_server.php')"><?echo I18N("h","Virtual Server");?></a></li>			
			<li class="dive"><a href="javascript:jumpto('/help/help_net_port_forwarding.php')"><?echo I18N("h","Port Forwarding");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_net_dynamic_dns.php')"><?echo I18N("h","Dynamic DNS");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_net_routing.php')"><?echo I18N("h","Routing");?></a></li>
		</ul>
		<h3 class="menuheader_mid expandable"><?echo I18N("h","Wireless");?></h3>
		<ul class="categoryitems">
			<li class="dive"><a href="javascript:jumpto('/help/help_wl_settings.php')"><?echo I18N("h","Wireless Settings");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_wl_advance.php')"><?echo I18N("h","Advanced Wireless");?></a></li>
		</ul>
		<h3 class="menuheader_mid expandable"><?echo I18N("h","Router");?></h3>
		<ul class="categoryitems">
			<li class="dive"><a href="javascript:jumpto('/help/help_rt_admin.php')"><?echo I18N("h","Administrator Settings");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_mt_firmware.php')"><?echo I18N("h","Firmware Update");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_rt_time.php')"><?echo I18N("h","Time");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_rt_email.php')"><?echo I18N("h","Email Settings");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_mt_system.php')"><?echo I18N("h","System");?></a></li>
		</ul>
		<h3 class="menuheader_mid expandable"><?echo I18N("h","Security");?></h3>
		<ul class="categoryitems">
			<li class="dive"><a href="javascript:jumpto('/help/help_sec_application_rules.php')"><?echo I18N("h","Application Rules");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_sec_firewall.php')"><?echo I18N("h","Firewall Settings");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_sec_mac_filter.php')"><?echo I18N("h","MAC Address Filtering");?></a></li>
		</ul>
		<h3 class="menuheader_end expandable"><?echo I18N("h","Log");?></h3>
		<ul class="categoryitems stem_end">
			<li class="dive"><a href="javascript:jumpto('/help/help_log_system_check.php')"><?echo I18N("h","System Check");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_log_device_info.php')"><?echo I18N("h","Device Info");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_log_info.php')"><?echo I18N("h","Logs");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_log_statistic.php')"><?echo I18N("h","Statistics");?></a></li>
			<li class="dive"><a href="javascript:jumpto('/help/help_log_internet_sessions.php')"><?echo I18N("h","Internet Sessions");?></a></li>
			<li class="dive_end"><a href="javascript:jumpto('/help/help_log_wireless_status.php')"><?echo I18N("h","Wireless");?></a></li>
		</ul>
	</li>
	<!-- left Menu -->
	<!-- Main content -->
	<li class="maincolumn_content">
	<script language="javascript">
	<!--
	
	//Drop-down Document Viewer II- © Dynamic Drive (www.dynamicdrive.com)
	//For full source code, 100's more DHTML scripts, and TOS,
	//visit http://www.dynamicdrive.com
	
	//Specify display mode (0 or 1)
	//0 causes document to be displayed in an inline frame, while 1 in a new browser window
	var displaymode=0
	//if displaymode=0, configure inline frame attributes (ie: dimensions, intial document shown
	var iframecode='<iframe id="external" style="width:100%; height:460px" frameborder="0" src="/help/help_net_wan.htm"></iframe>'
	
	/////NO NEED TO EDIT BELOW HERE////////////
	
	if (displaymode==0)
		document.write(iframecode)

	/* show current help information */
	var my_help = "/help/" + "<? echo $TEMP_MYHELP; ?>" + ".php";
	jumpto(my_help);
	
	function jumpto(inputurl){
		objs = document.getElementsByTagName('a');
		for (var i=0; i<objs.length; i++)
		{
			if (objs[i].href.indexOf(inputurl)>0)
				objs[i].className = "menu_selected";
			else
				objs[i].className = "";
		}
		if (document.getElementById&&displaymode==0)
			document.getElementById("external").src=inputurl
		else if (document.all&&displaymode==0)
			document.all.external.src=inputurl
		else{
			if (!window.win2||win2.closed)
				win2=window.open(inputurl)
					//else if win2 already exists
			else{
				win2.location=inputurl
					win2.focus()
			}
		}
		if (document.all) document.all.external.src=inputurl;
	}
	//-->
	</script>
	</li><br clear="all" />
	<!-- Main content -->
</ol>
</body>
</html>
