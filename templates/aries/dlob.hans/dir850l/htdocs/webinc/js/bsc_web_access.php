<?
/* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/inf.php";
//include "/htdocs/phplib/mdnsresponder.php";

function http_error($errno)
{
	fwrite("a", $_GLOBALS["START"], "exit ".$errno."\n");
	fwrite("a", $_GLOBALS["STOP"],  "exit ".$errno."\n");
}

function get_router_ip($name)
{
	/* Get the interface */
	$infp = XNODE_getpathbytarget("", "inf", "uid", $name, 0);

	if ($infp=="")
	{
		SHELL_info($_GLOBALS["START"], "webaccesssetup: (".$name.") not exist.");
		SHELL_info($_GLOBALS["STOP"],  "webaccesssetup: (".$name.") not exist.");
		http_error("9");
		return;
	}

	/* Get the "runtime" physical interface */
	$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $name, 0);

	if ($stsp!="")
	{
		$phy = query($stsp."/phyinf");
		if ($phy!="")
		{
			$phyp = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $phy, 0);
			if ($phyp!="" && query($phyp."/valid")=="1")
				$ifname = query($phyp."/name");
		}
	}

	/* Get address family & IP address */
	$atype = query($stsp."/inet/addrtype");

	if      ($atype=="ipv4") {$af="inet"; $ipaddr=query($stsp."/inet/ipv4/ipaddr");}
	else if ($atype=="ppp4") {$af="inet"; $ipaddr=query($stsp."/inet/ppp4/local");}
	else if ($atype=="ipv6") {$af="inet6";$ipaddr=query($stsp."/inet/ipv6/ipaddr");}
	else if ($atype=="ppp6") {$af="inet6";$ipaddr=query($stsp."/inet/ppp6/local");}

	if($af != "inet")
	{
		SHELL_info($_GLOBALS["START"], "webaccesssetup: (".$name.") not ipv4.");
		SHELL_info($_GLOBALS["STOP"],  "webaccesssetup: (".$name.") not ipv4.");
		http_error("9");
		return;
	}

	if ($ifname==""||$af==""||$ipaddr=="")
	{
		SHELL_info($_GLOBALS["START"], "webaccesssetup: (".$name.") no phyinf.");
		SHELL_info($_GLOBALS["STOP"],  "webaccesssetup: (".$name.") no phyinf.");
		http_error("9");
		return;
	}
	
	return $ipaddr;
}

function get_ddns_status($name)
{
	/* Get the "runtime" physical interface */
	$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $name, 0);

	if ($stsp!="")
	{
		$valid = query($stsp."/ddns4/valid");
		$status = query($stsp."/ddns4/status");
		if ($valid == "1")
		{
			return query($stsp."/ddns4/result");
		}
	}
	return "FAIL";
}
?>

<style>
/* The CSS is only for this page.
 * Notice:
 *	If the items are few, we put them here,
 *	If the items are a lot, please put them into the file, htdocs/web/css/$TEMP_MYNAME.css.
 */
#overlay_0
{
	z-index:10;
	background-color:black;
	position:absolute;
	top:0px;
	left:0px;
	width:100%;
	height:100%;
	opacity:.5;
	-moz-opacity:.5;
	-khtml-opacity:.5;
	filter:alpha(opacity=50);
	opacity:.50;
	display:none;
}
</style>
<script type="text/javascript">
function Page() {}
Page.prototype =
{
	<?
		if(isfile("/etc/services/UPNPC.php")==1) echo 'services: "WEBACCESS,INET.LAN-1,DDNS4.WAN-1,UPNPC",';
		else echo 'services: "WEBACCESS,INET.LAN-1,DDNS4.WAN-1",';
	?>
	OnLoad: function() {},
	OnUnload: function() {}, 
	OnSubmitCallback: function (){},
	InitValue: function(xml)
	{
		PXML.doc = xml;
		this.webaccess_p = PXML.FindModule("WEBACCESS");
		this.access_rule_org = new Array();
		this.access_rule = new Array();
		OBJ("en_webaccess").checked = COMM_ToBOOL(XG(this.webaccess_p+"/webaccess/enable"));
		OBJ("webaccess_httpport").value = XG(this.webaccess_p+"/webaccess/httpport");
		OBJ("webaccess_httpsport").value = XG(this.webaccess_p+"/webaccess/httpsport");	
		OBJ("en_webaccess_remote").checked = COMM_ToBOOL(XG(this.webaccess_p+"/webaccess/remoteenable"));
		if (!this.Initial()) return false;
		if (!this.InitOverlay()) return false;
		if (!this.InitEditWin()) return false;
		this.InitWebAccessLink();
		this.EnWebAccess();
		this.UserSelectModify();
		this.httplink();
		this.httpslink();
		
		return true;
	},
	Initial: function()
	{
		// Build the user list array
		var user_n = S2I(XG(this.webaccess_p+"/webaccess/account/entry#"));
		var h=0;
		for (var i=1; i <= user_n; i++)
		{
			var p = this.webaccess_p + "/webaccess/account/entry:" + i;
			var path_n = S2I(XG(p+"/entry#"));			
			for (var j=1; j <= path_n; j++)
			{
				this.access_rule_org[h] = {
					name:		XG(p+"/username"),
					passwd:		XG(p+"/passwd"),
					path:		XG(p+"/entry:"+j+"/path"),
					permission:	XG(p+"/entry:"+j+"/permission")
				};
				this.access_rule[h] = {
					name:		XG(p+"/username"),
					passwd:		XG(p+"/passwd"),
					path:		XG(p+"/entry:"+j+"/path"),
					permission:	XG(p+"/entry:"+j+"/permission")
				};				
				h++;
			}
			
		}

		this.InsectUserListTable();
		this.InsectDeviceTable();
		return true;
	},	
	InitOverlay: function()
	{		
		var newOverlay = document.createElement("div");
		newOverlay.id = 'overlay_0';
		newOverlay.className = 'overlay_0';
		newOverlay.style.display = 'none';
		newOverlay.style.backgroundColor = "gray";
		newOverlay.style.width = getPageWidth() + 'px';
		newOverlay.style.height = getPageHeight() + 'px';
		document.body.insertBefore(newOverlay, document.body.firstChild);
	
		initOverlay("white");
		return true;
	},		
	InitEditWin: function()
	{
		var EditWinWidth = 500;
		var EditWinHeight = 200;
		var EditWin = document.createElement("div");
		EditWin.id="edit_window";
		EditWin.className="edit_window";
		EditWin.style.zIndex=11;
		EditWin.style.position = "absolute";
		EditWin.style.top = (getPageHeight()/2) - (EditWinHeight/2)+ "px";
		EditWin.style.left = (getPageWidth()/2) - (EditWinWidth/2)+ "px";
		EditWin.style.width = EditWinWidth + "px";
		EditWin.style.height = EditWinHeight + "px";
		EditWin.style.display = 'none';		
		EditWin.style.backgroundColor = "white";
		
		var EditWin_html = "<div class='blackbox'>";
		EditWin_html += "<h2>"+"<?echo i18n('Append new Folder');?>"+"</h2>";
		EditWin_html += "<div class='textinput'><span class='name'>"+"<?echo i18n('User Name');?>"+"</span><span class='delimiter'>:</span><span class='value'><input id='username_edit' type='text' disabled/></span></div><div class='gap'></div>";
		EditWin_html += "<div class='textinput'><span class='name'>"+"<?echo i18n('Folder');?>"+"</span><span class='delimiter'>:</span><span class='value'><input id='the_sharepath' type='text' disabled/>&nbsp;&nbsp;<input id='path_browse' type='button' value='"+"<?echo i18n('Browse');?>"+"' onClick='PAGE.OnAccessRuleEdit_browse();'/>&nbsp;&nbsp;<input id='root_path_check' type='checkbox' onClick='PAGE.OnAccessRuleEdit_root();'/>"+"<?echo i18n('root');?>"+"</span></div><div class='gap'></div>";
		EditWin_html += "<div class='textinput'><span class='name'>"+"<?echo i18n('Permission');?>"+"</span><span class='delimiter'>:</span><span class='value'><select id='permission_edit'><option value='ro'>"+"<?echo i18n('Read Only');?>"+"</option><option value='rw'>"+"<?echo i18n('Read/Write');?>"+"</option></select></span></div><div class='gap'></div>";
		EditWin_html += "<div class='centerline'><input id='accessrule_append' type='button' style='width:100px;' value='"+"<?echo i18n('Append');?>"+"' onClick='PAGE.OnAccessRuleEdit_append();'/></div><div class='gap'></div>";
		EditWin_html += "<div class='centerline'><input type='button' value='"+"<?echo i18n('OK');?>"+"' onClick='PAGE.OnAccessRuleEdit_ok();'/>&nbsp;&nbsp;<input type='button' value='"+"<?echo i18n('Cancel');?>"+"' onClick='PAGE.OnAccessRuleEdit_cancel();'/></div><div class='gap'></div>";
		EditWin_html += "</div>";		
		EditWin.innerHTML=EditWin_html;	
		
		document.body.appendChild(EditWin);

		return true;	
	},
	PreSubmit: function()
	{		
		if(!this.CheckAccessPort()) return null;	
		XD(this.webaccess_p+"/webaccess/account");
		
		var user_n = 1;
		var user_count = 1;
		var path_n = 1;
		var i=0;
		
		for(i=0; i < this.access_rule.length; i++)
		{
			if(this.access_rule[i].path.toLowerCase() == "none")
			{
				BODY.ShowAlert("The Access Path of User [ "+ this.access_rule[i].name +" ] can not be \"None\"");
				return null;
			}
		}
		//check user path
		
		for(i=0; i < this.access_rule.length; i++)
		{
			if(i===0)
			{ 
				XS(this.webaccess_p+"/webaccess/account/entry:"+user_n+"/username", this.access_rule[i].name);
				XS(this.webaccess_p+"/webaccess/account/entry:"+user_n+"/passwd", this.access_rule[i].passwd);
				XS(this.webaccess_p+"/webaccess/account/entry:"+user_n+"/entry:"+path_n+"/path", this.access_rule[i].path);
				XS(this.webaccess_p+"/webaccess/account/entry:"+user_n+"/entry:"+path_n+"/permission", this.access_rule[i].permission);
			}
			else
			{
				if(this.access_rule[i].name === this.access_rule[i-1].name)
				{
					path_n++;
					XS(this.webaccess_p+"/webaccess/account/entry:"+user_n+"/entry:"+path_n+"/path", this.access_rule[i].path);
					XS(this.webaccess_p+"/webaccess/account/entry:"+user_n+"/entry:"+path_n+"/permission", this.access_rule[i].permission);					
				}
				else
				{
					user_count++;
					user_n++;
					path_n = 1;
					XS(this.webaccess_p+"/webaccess/account/entry:"+user_n+"/username", this.access_rule[i].name);
					XS(this.webaccess_p+"/webaccess/account/entry:"+user_n+"/passwd", this.access_rule[i].passwd);
					XS(this.webaccess_p+"/webaccess/account/entry:"+user_n+"/entry:"+path_n+"/path", this.access_rule[i].path);
					XS(this.webaccess_p+"/webaccess/account/entry:"+user_n+"/entry:"+path_n+"/permission", this.access_rule[i].permission);
				}					
			}	
		}
		XS(this.webaccess_p+"/webaccess/account/count", user_count);
		
		XS(this.webaccess_p+"/webaccess/enable", OBJ("en_webaccess").checked?1:0);
		XS(this.webaccess_p+"/webaccess/httpenable", OBJ("en_webaccess_remote").checked?1:0);		
		XS(this.webaccess_p+"/webaccess/httpport", OBJ("webaccess_httpport").value);
		XS(this.webaccess_p+"/webaccess/httpsenable", OBJ("en_webaccess_remote").checked?1:0);		
		XS(this.webaccess_p+"/webaccess/httpsport", OBJ("webaccess_httpsport").value);
		XS(this.webaccess_p+"/webaccess/remoteenable", OBJ("en_webaccess_remote").checked?1:0);
		return PXML.doc;
	},
	CheckAccessPort: function()
	{
		//The port 21,23,80,143,443 and LAN interface are bound already. It should not bind the same port and interface for this service.
		if (!TEMP_IsDigit(OBJ("webaccess_httpport").value) || OBJ("webaccess_httpport").value==23  || OBJ("webaccess_httpport").value==80 || OBJ("webaccess_httpport").value==443 || OBJ("webaccess_httpport").value==21 || OBJ("webaccess_httpport").value==143 || OBJ("webaccess_httpport").value==8182 || OBJ("webaccess_httpport").value==8183)
		{
			BODY.ShowAlert("<?echo i18n("The HTTP Access Port number is not valid.");?>");
			return false;
		}
		if (!TEMP_IsDigit(OBJ("webaccess_httpsport").value) || OBJ("webaccess_httpsport").value==80 || OBJ("webaccess_httpsport").value==443 || OBJ("webaccess_httpsport").value==21 || OBJ("webaccess_httpsport").value==143 || OBJ("webaccess_httpsport").value==8182 ||OBJ("webaccess_httpsport").value==8183)
		{
			BODY.ShowAlert("<?echo i18n("The HTTPS Access Port number is not valid.");?>");
			return false;
		}				
		if(OBJ("webaccess_httpport").value == OBJ("webaccess_httpsport").value)
		{
			BODY.ShowAlert("<?echo i18n("The HTTP Access Port could not be the same as the HTTPS Access Port.");?>");
			return false;
		}
		if(!this.PortConflictCheck()) return false;		
		return true;	
	},
	PortConflictCheck: function()
	{
		//Check the port confliction between STORAGE, VSVR, PFWD and REMOTE.
		var admin_remote_port = "<? echo query(INF_getinfpath("WAN-1")."/web");?>";
		var admin_remote_port_https = "<? echo query(INF_getinfpath("WAN-1")."/https_rport");?>";
		if(OBJ("webaccess_httpport").value==admin_remote_port || OBJ("webaccess_httpport").value==admin_remote_port_https ||
			OBJ("webaccess_httpsport").value==admin_remote_port || OBJ("webaccess_httpsport").value==admin_remote_port_https)
		{
			BODY.ShowAlert("<?echo i18n("The web access port could not be the same as the remote admin port in the administration function.");?>");
			return false;
		}
		<?
			$vsvr_port_str = "";
			foreach ("/nat/entry:1/virtualserver/entry")
			{
				if(query("protocol")=="TCP" || scut_count(query("protocol"),"TCP")!=0)
				{ $vsvr_port_str = $vsvr_port_str.",".query("external/start")."-".query("external/end");}
				if(query("tport_str")!="")
				{ $vsvr_port_str = $vsvr_port_str.",".query("tport_str");}	
			}
			$pfwd_port_str = "";
			foreach ("/nat/entry:1/portforward/entry")
			{
				if(query("protocol")=="TCP" || scut_count(query("protocol"),"TCP")!=0)
				{ $pfwd_port_str = $pfwd_port_str.",".query("external/start")."-".query("external/end");}	
				if(query("tport_str")!="")
				{ $pfwd_port_str = $pfwd_port_str.",".query("tport_str");}	
			}				
		?>
		var pfwd_port_str = "<? echo $pfwd_port_str;?>";
		var vsvr_port_str = "<? echo $vsvr_port_str;?>";
		if(PortStringCheck(pfwd_port_str, OBJ("webaccess_httpport").value) || 
			PortStringCheck(pfwd_port_str, OBJ("webaccess_httpsport").value))
		{
			BODY.ShowAlert("<?echo i18n("The web access port had been used in the port forwarding function.");?>");
			return false;			
		}
		if(PortStringCheck(vsvr_port_str, OBJ("webaccess_httpport").value) || 
			PortStringCheck(vsvr_port_str, OBJ("webaccess_httpsport").value))
		{
			BODY.ShowAlert("<?echo i18n("The web access port had been used in the virtual server function.");?>");
			return false;			
		}		
		return true;		
	},			
	IsDirty: function()
	{
		if(this.access_rule_org.length !== this.access_rule.length) return true;
		else
		{
			for (var i=0; i < this.access_rule_org.length; i++)
			{
				if(this.access_rule_org[i].name!==this.access_rule[i].name || this.access_rule_org[i].passwd!==this.access_rule[i].passwd ||
					this.access_rule_org[i].path!==this.access_rule[i].path || this.access_rule_org[i].permission!==this.access_rule[i].permission)
					return true;
			}
		}
		if(COMM_IsDirty(false)) return true;
		return false;
	},
	Synchronize: function() {},
	// The above are MUST HAVE methods ...
	///////////////////////////////////////////////////////////////////////
	webaccess_ip: "",	
	webaccess_port: "",
	webaccess_sip: "",	
	webaccess_sport: "",	
	client_in_lan: null,
	webaccess_p: null,
	access_rule_org: null,
	access_rule: null,
	access_rule_i: null,
	user_name_array: new Array(),
	path_modify_array: new Array(),
	ddns_active: "",
	ddns_host: "",
	ddns_st: "FAIL",
	InsectUserListTable: function()
	{
		BODY.CleanTable("userlisttable");
		this.PathModify_v1p03();
		var no = 1;
		var num = 1;
		for(var i=0; i < this.access_rule.length; i++)
		{
			var username = this.access_rule[i].name;
			if(i > 0)
			{ 
				if(this.access_rule[i].name === this.access_rule[i-1].name)
				{
					no = "";
					username = "";
				}
				else
				{ 
					num++;
					no = num;
				}	
			}
			if(this.access_rule[i].permission==="rw") var permission = "<?echo I18N("j","Read/Write");?>";
			else var permission = "<?echo I18N("j","Read Only");?>";
			var edit_html = '<a href="javascript:PAGE.OnAccessRuleEdit('+i+');"><img src="pic/img_edit.gif" alt="<?echo I18N("h", "Edit");?>"></a>';
			var delet_html = '<a href="javascript:PAGE.OnAccessRuleDelete('+i+');"><img src="pic/img_delete.gif" alt="<?echo I18N("h", "Delete");?>"></a>';
			if (username.toLowerCase() === "admin")
			{
				edit_html = "";
				delet_html = "";
			} 				
			var data = [no, username, this.path_modify_array[i], permission, edit_html, delet_html];
			var type = ["text","text","text","text","",""];
			BODY.InjectTable("userlisttable", "accessrule"+i, data, type);	
		}
	},
	InsectDeviceTable: function()
	{
		BODY.CleanTable("devicetable");
		var partition_num=0;
		var device_n = S2I(XG(this.webaccess_p+"/webaccess/device/entry#"));
		for(var i=1; i <= device_n; i++)
		{
			if(XG(this.webaccess_p+"/webaccess/device/entry:"+i+"/valid")==="1")
			{
				var partition_n = S2I(XG(this.webaccess_p+"/webaccess/device/entry:"+i+"/entry#"));
				for(var j=1; j <= partition_n; j++)
				{
					var partition_name = XG(this.webaccess_p+"/webaccess/device/entry:"+i+"/entry:"+j+"/uniquename");
					var total_space = XG(this.webaccess_p+"/webaccess/device/entry:"+i+"/entry:"+j+"/space/size");
					total_space = Volume_Unit(total_space);
					var free_space = XG(this.webaccess_p+"/webaccess/device/entry:"+i+"/entry:"+j+"/space/available");
					free_space = Volume_Unit(free_space);
					var data = [partition_name, total_space, free_space];
					var type = ["text","text","text"];
					BODY.InjectTable("devicetable", "partition"+partition_num, data, type);
					partition_num++;			
				}
			}				
		}
		OBJ("devicen").innerHTML = "<?echo i18n('Number Devices');?>"+ ":" + partition_num;
	},	
	OnAccessRuleEdit: function(i)
	{
		this.access_rule_i = i;
		OBJ("edit_window").style.display = 'block';
		OBJ("username_edit").value = this.access_rule[i].name;
		if(this.access_rule[i].path=="root")
		{
			OBJ("root_path_check").checked = true;
			OBJ("path_browse").disabled = true;
		}
		else
		{
			OBJ("root_path_check").checked = false;
			OBJ("path_browse").disabled = false;
		}
		OBJ("the_sharepath").value = this.access_rule[i].path;
		COMM_SetSelectValue(OBJ("permission_edit"), this.access_rule[i].permission);
		OBJ("overlay_0").style.display = 'block';
		

		var path_num = 0;
		OBJ("accessrule_append").disabled=false;
		for(var j=0; j < this.access_rule.length; j++)
		{
			if(this.access_rule[j].name===OBJ("username_edit").value) path_num++;
			//The max access rule for a user is 5.
			if(path_num > 4) OBJ("accessrule_append").disabled=true;
		}
	},
	OnAccessRuleEdit_browse: function()
	{
		window_make_new(-1, -1, 500, 400, "portal/explorer.php?path=/","Explorer");
	},	
	OnAccessRuleEdit_append: function()
	{
		this.access_rule.splice(this.access_rule_i+1, 0, {name:this.access_rule[this.access_rule_i].name, passwd:this.access_rule[this.access_rule_i].passwd, path:this.OnAccessRuleEdit_PathModify(), permission:OBJ("permission_edit").value});
		this.InsectUserListTable();
		this.OnAccessRuleEdit_cancel();		
	},		
	OnAccessRuleEdit_ok: function()
	{		
		this.access_rule[this.access_rule_i].path = this.OnAccessRuleEdit_PathModify();
		this.access_rule[this.access_rule_i].permission = OBJ("permission_edit").value;
		this.InsectUserListTable();
		this.OnAccessRuleEdit_cancel();
	},
	OnAccessRuleEdit_cancel: function()
	{
		OBJ("edit_window").style.display = "none";
		OBJ("overlay_0").style.display = "none";			
	},
	OnAccessRuleEdit_PathModify: function()
	{
		//Modify the device name in path to unique device name
		var path_modify = "";
		var uniquename = "";
		if(OBJ("the_sharepath").value.match(":")==":") path_modify = OBJ("the_sharepath").value;
		else if(OBJ("root_path_check").checked)
		{
			path_modify = "root";
		}		
		else
		{
			var mntp_name = OBJ("the_sharepath").value.split("/",1);
			var device_n = S2I(XG(this.webaccess_p+"/webaccess/device/entry#"));
			for (var i=1; i <= device_n; i++)
			{
				var partition_n = S2I(XG(this.webaccess_p+"/webaccess/device/entry:"+i+"/entry#"));			
				for (var j=1; j <= partition_n; j++)
				{
					var p = this.webaccess_p+"/webaccess/device/entry:"+i+"/entry:"+j;
					var mntp = XG(p+"/mntp");
					if(mntp.match(mntp_name)!==null) uniquename = XG(p+"/uniquename");
				}
			}
			if(uniquename!=="") path_modify = OBJ("the_sharepath").value.replace(mntp_name, uniquename+":");
			else path_modify = "None";
		}
		return path_modify;	
	},						
	OnAccessRuleDelete: function(i)
	{
	/*By DLINK request,When existed only one Guest account and then clicked "Delete" button,reset Guest account settings to be default and poped up warning message.*/
		var check=0;
		var username="";
		/*   //guest is no longer default account , so we have to mark below checkment to make sure delete "guest" would work by jef 20121206
		for(var x=0;  x < this.access_rule.length;x++)
		{
			username = this.access_rule[x].name;
			if( username.toLowerCase() == "guest" )
			{//check means how many guest account exists.
				check++;
			}
		}
		if( (OBJ("accessrule"+i+"_1").innerHTML).toLowerCase() == "guest" && check==1)
		{
			this.access_rule[i].passwd = "" ;
			this.access_rule[i].path = "none";
			this.access_rule[i].permission = "ro";
			BODY.ShowAlert("<?echo i18n("Restore To Factory Default Settings");?>");
		}
		else*/
		{
			this.access_rule.splice(i,1);
		}
		this.InsectUserListTable();
		this.UserSelectModify();
	},
	OnAccessRuleEdit_root: function()
	{
		if(OBJ("root_path_check").checked)
		{
			OBJ("path_browse").disabled = true;
			OBJ("the_sharepath").value = "root";
		}
		else
		{
			OBJ("path_browse").disabled = false;
			OBJ("the_sharepath").value = "None";
		}
	},		
	UserCreate: function()
	{			
		//+++ Jerry Kao, limit the number of max_user_list equals to 10.
		var max_usr_list = 10;
		
		if(OBJ("user_name").value !== "")
		{
			//Check user_name
			var user = OBJ("user_name").value;
			if(user.match(/[^a-z^A-Z^0-9]/g))
			{
				BODY.ShowAlert("<?echo I18N("j","The username is invalid");?>");
				return false;
			}
			
			//Check password
			var passwd = OBJ("pwd").value;
			if (passwd!==OBJ("pwd_verify").value)
			{
				BODY.ShowAlert("<?echo i18n("Please make the two passwords the same and try again.");?>");
				return false;
			}
			/* The IE browser would treat the text with all spaces as empty according as 
				it would ignore the text node with all spaces in XML DOM tree for IE6, 7, 8, 9.*/		
			if(COMM_IsAllSpace(passwd))
			{
				BODY.ShowAlert("<?echo i18n("Invalid Password.");?>");
				return false;
			}			
		for(var i=0;i < passwd.length;i++)
			{
				if (passwd.charCodeAt(i) > 256) //avoid holomorphic word
				{ 
					BODY.ShowAlert("<?echo i18n("Invalid Password.");?>");
					return false;
				}
				/*	if (passwd.charCodeAt(i) == 44) //The character "," is used to separate each parameter in the file /var/run/storage_account_root.
				{ 
					BODY.ShowAlert("<?echo i18n("Invalid Password.");?>");
					return false;
				}*/				
			}
			var new_user = true;
			for(var i=0;i < this.access_rule.length;i++)
			{
				if(this.access_rule[i].name === OBJ("user_name").value)
				{
					new_user = false;
					this.access_rule[i].passwd = passwd;
				}	 
			}
			
			if (this.access_rule.length >= max_usr_list)
			{
				new_user = false;
				BODY.ShowAlert("<?echo i18n("The rules exceed maximum.");?>");					
			}			
			
			if(new_user === true)
			{
				this.access_rule.push({name:OBJ("user_name").value, passwd:passwd, path:"None", permission:"ro"});
				this.InsectUserListTable();
			}
			OBJ("user_name").value = "";
			OBJ("pwd").value = "";
			OBJ("pwd_verify").value = "";
		}
		else
		{
					BODY.ShowAlert("<?echo i18n("Please input the User Name.");?>");
					return false;
		}		
		this.UserSelectModify();
	},
	UserChange: function()
	{
		OBJ("user_name").value = this.user_name_array[OBJ("user_edit").value];
		COMM_SetSelectValue(OBJ("user_edit"), 0);
	},
	UserSelectModify: function()
	{
		OBJ("user_select").parentNode.removeChild(OBJ("user_select"));
		this.user_name_array = [];
		this.user_name_array.push("<?echo i18n("User Name");?>");
		var str_user = "";
		str_user += '<select id="user_edit" onchange="PAGE.UserChange()">';
		str_user += '<option value=0>'+this.user_name_array[0]+'</option>';
		var j=1; // for this.user_name_array
		for(var i=0;i < this.access_rule.length;i++)
		{
			if(this.access_rule[i].name.toLowerCase()=="admin") continue;			
			else if(i===0)
			{
				this.user_name_array[1] = this.access_rule[0].name;
				str_user += '<option value=1>'+this.user_name_array[1]+'</option>';
				j++;
			}
			else if(this.access_rule[i].name !== this.access_rule[i-1].name)
			{
				this.user_name_array[j] = this.access_rule[i].name;
				str_user += '<option value='+j+'>'+this.user_name_array[j]+'</option>';
				j++;		
			}		
		}
		str_user += '</select>';
		var userselet = document.createElement("span");
		userselet.id = "user_select";
		userselet.innerHTML = str_user;
		OBJ("user_choose").appendChild(userselet);
	},	
	EnWebAccess: function()
	{
		if(OBJ("en_webaccess").checked)
		{
			OBJ("webaccess_httpport").disabled = false;
			OBJ("webaccess_httpsport").disabled = false;
			OBJ("en_webaccess_remote").disabled = false;
		}
		else
		{
			OBJ("webaccess_httpport").disabled = true;
			OBJ("webaccess_httpsport").disabled = true;
			OBJ("en_webaccess_remote").disabled = true;
		}	
		if(!OBJ("en_webaccess_remote").checked)
		{
			document.getElementById("showlink_http").style.display = "none"; 
			document.getElementById("showlink_ssl").style.display = "none";
		}
		else if(OBJ("webaccess_httpport").value!="" || OBJ("webaccess_httpsport").value!="")
		{
			if(OBJ("webaccess_httpport").value!="") document.getElementById("showlink_http").style.display = "block";
			if(OBJ("webaccess_httpsport").value!="") document.getElementById("showlink_ssl").style.display = "block";
			
		}
		else
		{
			document.getElementById("showlink_http").style.display = "none";
			document.getElementById("showlink_ssl").style.display = "none"; 
		}		
	},
	//==20130123 jack modify to "not" show shareport link without wan link==//
	httplink: function()
	{
		if(this.ddns_st == "SUCCESS" && this.ddns_host != "")
			OBJ("http_link").innerHTML = "<a href='" + "http://"+ this.ddns_host + ":" + this.webaccess_port + "'" + " target='_blank'>" + "http://" + this.ddns_host + ":" + this.webaccess_port + "</a>";			
		else if(this.webaccess_ip != "")
			OBJ("http_link").innerHTML = "<a href='" + "http://"+ this.webaccess_ip + ":" + this.webaccess_port + "'" + " target='_blank'>" + "http://" + this.webaccess_ip + ":" + this.webaccess_port + "</a>";
		else
			OBJ("http_link").innerHTML = "";
	},
	httpslink: function()
	{
		if(this.ddns_st == "SUCCESS" && this.ddns_host != "")
			OBJ("https_link").innerHTML = "<a href='" + "https://" + this.ddns_host+ ":" + this.webaccess_sport + "'" + " target='_blank'>" + "https://" + this.ddns_host+ ":" + this.webaccess_sport + "</a>";
		else if(this.webaccess_sip != "")
			OBJ("https_link").innerHTML = "<a href='" + "https://" + this.webaccess_sip+ ":" + this.webaccess_sport + "'" + " target='_blank'>" + "https://" + this.webaccess_sip+ ":" + this.webaccess_sport + "</a>";
		else
			OBJ("https_link").innerHTML = "";
	},
	InitWebAccessLink: function()
	{
		var routerWanIp = "<?echo get_router_ip("WAN-1");?>";
		var clientIp = "<?echo $_SERVER["REMOTE_ADDR"];?>";
		var lan	= PXML.FindModule("INET.LAN-1");
		var inetuid = XG  (lan+"/inf/inet");		
		var inetp = GPBT(lan+"/inet", "entry", "uid", inetuid, false);		
		if (!inetp)
		{
			BODY.ShowAlert("<?echo i18n("InitLAN() ERROR!!!");?>");
			return false;
		}
		if (XG  (inetp+"/addrtype") == "ipv4")
		{
			var b = inetp+"/ipv4";
			var routerLanIp = XG  (b+"/ipaddr");
			var mask = XG  (b+"/mask");
		}
		/*  solve problem: remote webaccess link can't accessed by LAN user 
			(it's cause by our devie's architerture) */
		/*
		if( COMM_IPv4NETWORK(clientIp,mask) == COMM_IPv4NETWORK(routerLanIp,mask) ) //user in LAN side, give wfa local link
		{
			this.client_in_lan = 1;
			this.webaccess_ip = routerLanIp;
			this.webaccess_port = "8181";
			this.webaccess_sip = routerLanIp;
			this.webaccess_sport = "444";
		}
		else //user come from WAN side, give remote wfa link
		{
			this.client_in_lan = 0;
			this.webaccess_ip = routerWanIp;
			this.webaccess_port = OBJ('webaccess_httpport').value;
			this.webaccess_sip = routerWanIp;
			this.webaccess_sport = OBJ('webaccess_httpsport').value;
		}	
		*/
		this.webaccess_ip = routerWanIp;
		this.webaccess_port = OBJ('webaccess_httpport').value;
		this.webaccess_sip = routerWanIp;
		this.webaccess_sport = OBJ('webaccess_httpsport').value;
		
		//check ddns
		var p = PXML.FindModule("DDNS4.WAN-1");
		this.ddns_active = (XG(p+"/inf/ddns4")!="");
		var ddnsp = GPBT(p+"/ddns4", "entry", "uid", "DDNS4-1", 0);
		this.ddns_host = XG(ddnsp+"/hostname");
		if(this.ddns_active)
		{
			this.ddns_st = "<?echo get_ddns_status("WAN-1");?>";
		}		
		return true;
	},
	PathModify_v1p03: function()
	{
		//Modify the path according as D-Link Router Web File Access Specification v1.03
		//Modify the path name from "(1) /HD1/Path" to "(1) HD1:/Path". Discussion with Daniel.
		for(var i=0; i < this.access_rule.length; i++)
		{
			if(this.access_rule[i].path=="root") this.path_modify_array[i] = "/";
			else if(this.access_rule[i].path=="None") this.path_modify_array[i] = "None";
			else if(i==0)
			{
				//this.path_modify_array[i] = "(1) /" + this.access_rule[i].path.replace(":", "");
				this.path_modify_array[i] = "(1) " + this.access_rule[i].path;
			}		
			else
			{
				var m=1;
				for(var j=0; j < i; j++)
				{
					if(this.access_rule[i].name == this.access_rule[j].name) m++; 
				}
				//this.path_modify_array[i] = "("+m+") /" + this.access_rule[i].path.replace(":", "");
				this.path_modify_array[i] = "("+m+") " + this.access_rule[i].path;
			}		
		}
	}		
}
function Volume_Unit(vol)
{
	//Default unit is KB
	var vol_mod = "";
	var vol_num = parseInt(vol, 10);
	if(vol_num > Math.pow(10,12)) vol_mod = ">1PB";
	else if(vol_num > Math.pow(10,9))
	{
		if(vol.substr(vol.length-9 ,1)==="0") vol_mod = vol.substring(0, vol.length-9)+" TB";
		else vol_mod = vol.substring(0, vol.length-9)+"."+vol.substr(vol.length-9 ,1)+" TB";		
	}	
	else if(vol_num > Math.pow(10,6))
	{
		if(vol.substr(vol.length-6 ,1)==="0") vol_mod = vol.substring(0, vol.length-6)+" GB";
		else vol_mod = vol.substring(0, vol.length-6)+"."+vol.substr(vol.length-6 ,1)+" GB";		
	}	
	else if(vol_num > Math.pow(10,3))
	{
		if(vol.substr(vol.length-3 ,1)==="0") vol_mod = vol.substring(0, vol.length-3)+" MB";
		else vol_mod = vol.substring(0, vol.length-3)+"."+vol.substr(vol.length-3 ,1)+" MB";			
	}
	else if(vol_num < Math.pow(10,3) && vol_num > 1) vol_mod = vol+" KB";
	else if(vol_num < 1) vol_mod = "<1KB";
	return vol_mod;
}
function PortStringCheck(PortString, port)
{
	var PortStrArr = PortString.split(",");
	port = parseInt(port, 10)
	for(var i=0; i < PortStrArr.length; i++)
	{
		if(PortStrArr[i].match("-")=="-")
		{
			var PortRange = PortStrArr[i].split("-");
			var PortRangeStart	= parseInt(PortRange[0], 10);
			var PortRangeEnd	= parseInt(PortRange[1], 10);
			if(PortRangeStart <= port && port <= PortRangeEnd) return true;
		}	
		else if(PortStrArr[i]!="")
		{
			if(parseInt(PortStrArr[i], 10)==port) return true;
		}
	}
	return false;
}
</script>
