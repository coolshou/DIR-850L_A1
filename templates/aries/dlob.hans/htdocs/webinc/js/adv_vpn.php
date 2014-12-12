<script type="text/javascript">
var EventName = null;
function Page() {}
Page.prototype =
{
	services: "VPN",
	OnLoad: function() {},
	OnUnload: function() {},
	OnSubmitCallback: function ()	{},
	upnpav: null,
	InitValue: function(xml)
	{
		PXML.doc = xml;
		this.VPN_p = PXML.FindModule("VPN");
		this.VPN_p = this.VPN_p+"/vpn";
		
		OBJ("pptp_enable").checked = COMM_ToBOOL(XG(this.VPN_p+"/pptp"));
		
		var chap_enable = COMM_ToBOOL(XG(this.VPN_p+"/authtype/chap"));
		var mschap_enable = COMM_ToBOOL(XG(this.VPN_p+"/authtype/mschap"));
		var encrtype_enable = COMM_ToBOOL(XG(this.VPN_p+"/encrtype"));
		var isolation_enable = COMM_ToBOOL(XG(this.VPN_p+"/isolation"));
		if(!chap_enable && !mschap_enable) OBJ("auth_none").checked = true;
		else
		{
			OBJ("auth_chap").checked = chap_enable;
			OBJ("auth_mschap").checked = mschap_enable;
		}
		OBJ("encrypt_enable").checked = encrtype_enable;
		OBJ("isolation_enable").checked = isolation_enable;
		this.VPNenable();
		this.AUTHcheck();
		this.BuildUserTable();
		
		return true;
	},
	BuildUserTable: function()
	{
		BODY.CleanTable("usertable");
		var user_num = XG(this.VPN_p+"/account/entry#");
		for (var i=1; i<=user_num; i++)
		{
			var user_name = XG(this.VPN_p+"/account/entry:"+i+"/name");
			var user_password = XG(this.VPN_p+"/account/entry:"+i+"/password");
			var user_uid = "user_" + i;
			var data = [user_name, user_password,
				'<a href="javascript:PAGE.UserEdit('+i+');"><img src="pic/img_edit.gif" title="<?echo I18N("h", "Edit");?>"></a>',
				'<a href="javascript:PAGE.UserDelete('+i+');"><img src="pic/img_delete.gif" title="<?echo I18N("h", "Delete");?>"></a>'
				];
			var type = ["text","text","",""];
			BODY.InjectTable("usertable", user_uid, data, type);
		}		
	},
	UserEdit: function(i)
	{
		OBJ("account").value = XG(this.VPN_p+"/account/entry:"+i+"/name");
		OBJ("password").value = XG(this.VPN_p+"/account/entry:"+i+"/password");
		OBJ("user_add_edit").value = "<?echo i18n("Edit");?>";
		this.AccountModify = i;
		
		this.BuildUserTable();
	},
	UserDelete: function(i)
	{
		XD(this.VPN_p+"/account/entry:"+i);
		this.PageDirty = true;
		this.BuildUserTable();
	},			
	UserAddEdit: function()
	{
		if(this.AccountModify == null)
		{
			var new_user_idx = XG(this.VPN_p+"/account/entry#") + 1;
			XA(this.VPN_p+"/account/entry:"+new_user_idx+"/name", OBJ("account").value);
			XA(this.VPN_p+"/account/entry:"+new_user_idx+"/password", OBJ("password").value);			
		}
		else
		{		
			XS(this.VPN_p+"/account/entry:"+this.AccountModify+"/name", OBJ("account").value);
			XS(this.VPN_p+"/account/entry:"+this.AccountModify+"/password", OBJ("password").value);
		}
		OBJ("user_add_edit").value = "<?echo i18n("Add");?>";
		this.AccountModify = null;
		
		this.BuildUserTable();
	},	
	PreSubmit: function()
	{
		XS(this.VPN_p+"/pptp", COMM_ToNUMBER(OBJ("pptp_enable").checked));
		
		XS(this.VPN_p+"/authtype/chap", COMM_ToNUMBER(OBJ("auth_chap").checked));
		XS(this.VPN_p+"/authtype/mschap", COMM_ToNUMBER(OBJ("auth_mschap").checked));
		XS(this.VPN_p+"/encrtype", COMM_ToNUMBER(OBJ("encrypt_enable").checked));
		XS(this.VPN_p+"/isolation", COMM_ToNUMBER(OBJ("isolation_enable").checked));
		
		return PXML.doc;
	},
	VPN_p: null,
	PPTP_p: null,
	AccountModify: null,
	PageDirty: false,
	IsDirty: function() 
	{
		return this.PageDirty;
	},
	Synchronize: function() {},
	VPNenable: function() 
	{	
		if(OBJ("pptp_enable").checked) 
		{
			OBJ("account").disabled = OBJ("password").disabled = OBJ("user_add_edit").disabled = false;
			OBJ("auth_none").disabled = false;
			OBJ("isolation_enable").disabled = false;
			this.AUTHcheck();
		}
		else
		{	 
			OBJ("account").disabled = OBJ("password").disabled = OBJ("user_add_edit").disabled = true;
			OBJ("auth_none").disabled = OBJ("auth_chap").disabled = OBJ("auth_mschap").disabled = true;
			OBJ("encrypt_enable").disabled = true;
			OBJ("isolation_enable").disabled = true;
		}
	},
	AUTHcheck: function() 
	{		
		if(!OBJ("auth_none").checked) 
		{
			OBJ("auth_chap").disabled = OBJ("auth_mschap").disabled = false;
		}
		else
		{
			OBJ("auth_chap").checked = OBJ("auth_mschap").checked = false;
			OBJ("auth_chap").disabled = OBJ("auth_mschap").disabled = true;
		}
		this.MPPEcheck();
	},
	MPPEcheck: function() 
	{
		if(OBJ("auth_mschap").checked) 
		{
			OBJ("encrypt_enable").disabled = false;
		}
		else
		{
			OBJ("encrypt_enable").checked = false;
			OBJ("encrypt_enable").disabled = true;
		}		
	}	
}
</script>
