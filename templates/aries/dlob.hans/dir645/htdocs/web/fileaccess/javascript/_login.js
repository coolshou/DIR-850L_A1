
function Login() {}
Login.prototype =
{
	Post: function() {
		var p_name = pary[OBJ("loginusr").selectedIndex+1][0];
		this.setCookie("name", p_name);
		this.checkCookie();
	},
	getCookie: function(c_name) {
		if (document.cookie.length>0)
		{
			var c_list = document.cookie.split(";");
			for (var i=0;i<c_list.length-1;i++)
			{
				var cook = c_list[i].split("=");
				if ( cook[0].toString() == c_name.toString() )
				{
					return cook[1];
				}
			} 
		}
		return null
	},
	setCookie: function(c_name,value) {
		document.cookie = c_name+"="+value;
		return;
	},
	delCookie: function() {
		document.cookie = "name=";
		return;
	},
	checkCookie: function()
	{
		var cname = this.getCookie("name");
		if(cname==null || cname=='')
		{
			OBJ("login_screen").style.display = "";
			OBJ("screen").style.display = "none";
			OBJ("alertmsg_screen").style.display = "none";
		}
		else
		{
			OBJ("login_screen").style.display = "none";
			OBJ("screen").style.display = "";
			OBJ("alertmsg_screen").style.display = "none";
			
			for(var i=1;i<pary.length;i++)
			{
				if(pary[i][0]==cname)
				{
					var pathstr = pary[i][1].split(":");
					var volid = pathstr[0];
					
					API._SetValue(cname,volid,"aaa:555");
					Init_Default_Value("?id="+cname+"&volid="+volid+"&tok=aaa:555&path=");
					API._LoginSuccess(pathstr);
					break;
				}
			}
			// function main.
			API.InitValue();
		}
	}
}
