var OBJ	= COMM_GetObj;
function AlertMSG(string)
{
	OBJ("text_msg").innerHTML = string;
	OBJ("alertmsg_screen").style.display="";
	OBJ("screen").style.display="none";
}

function API() {}
API.prototype =
{
	auth_name: null,
	auth_pwd: null,
	auth_tok: null,
	auth_volid: null,
	user_type: CurrentType.Line, //1:line detail, 2:block(icon) detail
	//////////////////////////////////////////////////////////////
	InitValue: function() {
		GVar.AllPath = API.PreCurrentPath(GVar.PathAry);
		//init user interface, must different with current type.
		UserType(CurrentType.Icon, false);
		//alert("GVar.AllPath="+GVar.AllPath);
		API.Reload();
	},
	ErrorMsg2Reload: function() {
		OBJ("alertmsg_screen").style.display="none";
		OBJ("screen").style.display="";
		this.Reload();
	},
	Reload: function() {
		LoadIMGShow();
		if(API.user_type==CurrentType.Line)
			API.GetJson(GVar.PathAry, GVar.AllPath, REMOTE_URL.ListDir, REMOTE_URL.ListDirPath, API._Callback_Detail_ListDir);
		else
			API.GetJson(GVar.PathAry, GVar.AllPath, REMOTE_URL.ListDir, REMOTE_URL.ListDirPath, API._Callback_Block_ListDir);
	},
	PreCurrentPath: function(ary) {
		var str="";
		if(ary.length==1)
			str = ary[0];
		else
		{
			//ary[0] maybe '/' or '/aa'
			str = ary[0];	//skip first '/'
			for(var i=1;i<ary.length;i++)
				str += "/" + ary[i];
		}
		OBJ("currentpath").innerHTML = str;
		return str;
	},
	GetJson: function(ary, path, node, nodepath, callback) {
		API.GetJsonR(ary, path, node, nodepath, callback, '');
	},
	GetJsonR: function(ary, path, node, nodepath, callback, other_path) {
		var rdm = "&rdm="+new Date().getTime();
		var ajaxObj = GetAjaxObj(node);
		ajaxObj.createRequest();
		ajaxObj.onCallback = function (text)
		{
			ajaxObj.release();
			//alert(node);
			if(node==REMOTE_URL.ListDir)
			{
				if(API.user_type==CurrentType.Line)
					API.GetJson(GVar.PathAry, GVar.AllPath, REMOTE_URL.ListFile, REMOTE_URL.ListFilePath, API._Callback_Detail_ListFile);
				else
					API.GetJson(GVar.PathAry, GVar.AllPath, REMOTE_URL.ListFile, REMOTE_URL.ListFilePath, API._Callback_Block_ListFile);
			}
			if(callback) callback(text);
		}
		ajaxObj.requestMethod = "GET";
		ajaxObj.returnXml = false;
		ajaxObj.setHeader("Content-Type", "application/x-www-form-urlencoded");
		//alert("nodepath+path="+nodepath+path);
		ajaxObj.sendRequest(nodepath+encodeURIComponent(path)+other_path+rdm, "");
		
		LoadIMGShow();
	},
	_Callback_Block_ListDir: function(jsonText) {
		var json = eval('('+jsonText+')');
		if(json.status=="fail")
		{
			AlertMSG("Get directory fail.");
			return;
		}
		//alert("count="+json.count);
		var dirsHTML = "";
		dirsHTML = API._Print_Block_Title();
		for(var i=0;i<json.count;i++)
		{
			var name = json.dirs[i].name;
			var folderlink = '<a href="javascript:API.FolderLink(\''+name+'\')" title="'+name+'">';
			if(name.length>15)
			{
				if(name.length>30)
					var name_str = name.substring(0,15)+"<br> ... "+name.substring(name.length-10,name.length);
				else
					var name_str = name.substring(0,15)+"<br>"+name.substring(15,name.length);
			}
			else
				var name_str = name;
			
			dirsHTML += '<div class="block_img">';
			dirsHTML += folderlink;
			dirsHTML += '<img src="images/blockphoto/Folder.png" alt="'+name+'" class="link_img" style="opacity:0.5;" /><br>';
			dirsHTML += name_str;
			dirsHTML += '</a></div>';			
		}
		OBJ("blockcontainer").innerHTML = dirsHTML;
	},
	_Callback_Block_ListFile: function(jsonText) {
		var json = eval('('+jsonText+')');
		if(json.status=="fail")
		{
			AlertMSG("Get file fail.");
			return;
		}
		//alert("count="+json.count);
		var filesHTML = "";
		for(var i=0;i<json.count;i++)
		{
			var name = json.files[i].name;
			if(name.length>15)
			{
				if(name.length>30)
					var name_str = name.substring(0,15)+"<br> ... "+name.substring(name.length-10,name.length);
				else
					var name_str = name.substring(0,15)+"<br>"+name.substring(15,name.length);
			}
			else
				var name_str = name;
			
			filesHTML += '<div class="block_img">';
			filesHTML += API._createBlockFileLink(name,name_str);
			filesHTML += '</div>';			
		}
		OBJ("blockcontainer").innerHTML += filesHTML;
		// Important!! Init jQuery format!!!
		API._OnLoadSuccess();
		LoadIMGHide();
	},
	_Print_Block_Title: function() {
		var parent_folder_div = '';
		
		if(GVar.PathAry.length > 1)
		{
			parent_folder_div += '<div class="block_img">';
			parent_folder_div += '<a href="javascript:API.UpFolder()">';
			parent_folder_div += '<img src="images/blockphoto/Toolbar-Undo.png" alt="Previous" class="link_img" style="opacity:0.5;" /></a></div>';
			return parent_folder_div;
		}
		return '';
	},
	_Print_Detail_Title: function() {
		var always_header = '<div class="screen title"><div class="title1">File Name</div><div class="title3">Modified</div><div class="title2">Size</div></div>';
		var parent_folder_div = '';
		if(GVar.PathAry.length > 1)
		{
			parent_folder_div += '<div class="screen detail"><div class="detail0"></div>';
			parent_folder_div += '<div class="detail1" style="font-weight: bold;padding-left:20px;height:15px;"><a href="javascript:API.UpFolder()">..</a></div>';
			parent_folder_div += '<div class="detail3"></div><div class="detail2"></div></div>';
		}
		
		OBJ("detailcontainer").innerHTML = always_header+parent_folder_div;
	},
	_Callback_Detail_ListDir: function(jsonText) {
		var json = eval('('+jsonText+')');
		var dirsHTML = "";
		API._Print_Detail_Title();
		
		for(var i=0;i<json.count;i++)
		{
			var name = json.dirs[i].name;
			var dynheight = (name.length>65?" style=\"height:33px;\"":"");
			var folderlink = '<a href="javascript:API.FolderLink(\''+name+'\')">'+name+'</a>';
			var stime = API.DateToString(json.dirs[i].mtime);
			
			dirsHTML += '<div class="screen detail"'+dynheight+'><div class="detail0"><input type=checkbox value="" id="ckd_'+i+'" disabled="disabled" /></div>';//value="'+name+'" 
			dirsHTML += '<div class="detail1">'+folderlink+'</div>';
			dirsHTML += '<div class="detail3">'+stime+'</div><div class="detail2"></div></div>';
		}
		
		OBJ("detailcontainer").innerHTML += dirsHTML;
	},
	_Callback_Detail_ListFile: function(jsonText) {
		var json = eval('('+jsonText+')');
		var filesHTML = "";
		for(var i=0;i<json.count;i++)
		{
			var name = json.files[i].name;
			
			var folderlink = null;
			
			folderlink = API._createFileLink(name);
			var stime = API.DateToString(json.files[i].mtime);
			var fsize = API.SizeToString(json.files[i].size);
			var dynheight = (name.length>65?" style=\"height:33px;\"":"");
			
			filesHTML += '<div class="screen detail"'+dynheight+'><div class="detail0"><input type=checkbox value="" id="ckf_'+i+'" disabled="disabled" /></div>';
			filesHTML += '<div class="detail1">'+folderlink+'</div>';
			filesHTML += '<div class="detail3">'+stime+'</div><div class="detail2">'+fsize+'</div></div>';
		}
		
		OBJ("detailcontainer").innerHTML += filesHTML;
		
		// Important!! Init jQuery format!!!
		API._OnLoadSuccess();
		LoadIMGHide();
	},
	DateToString: function(mtime) {
		var updatetime = new Date(mtime);
		var stime = updatetime.getFullYear()+"/"+(updatetime.getMonth()+1)+"/"+updatetime.getDay()+" "+updatetime.getHours()+":"+updatetime.getMinutes();
		return stime;
	},
	SizeToString: function(size) {
		if(!size) {
	        return '0 Bytes';
	    }
	    var sizeNames = [' Bytes', ' KB', ' MB', ' GB', ' TB', ' PB', ' EB', ' ZB', ' YB'];
	    var i = Math.floor(Math.log(size)/Math.log(1024));
	    var p = (i > 1) ? 2 : 0;
	    return (size/Math.pow(1024, Math.floor(i))).toFixed(p) + sizeNames[i];
	},
	UpFolder: function() {
		if(GVar.PathAry.length>1)
		{
			GVar.PathAry.pop();
			GVar.AllPath = API.PreCurrentPath(GVar.PathAry);
			API.Reload();
		}
	},
	FolderLink: function(linkname) {
		//alert("linkname="+linkname);
		GVar.PathAry.push(linkname);
		GVar.AllPath = API.PreCurrentPath(GVar.PathAry);
		API.Reload();
	},
	_createBlockFileLink: function(linkname, name_string) {
		var fileslink = "";
		var linkstr = '["'+linkname+'"]';
		var download_str = '"'+REMOTE_URL.DownloadFilesPath+GVar.AllPath+'&filenames='+encodeURIComponent(linkstr)+'"';
		var caption = 'Download: <a href='+download_str+'>'+linkname+'</a>';
			
		var myImageCheck = new lightwindow();
		if( linkname.substring(linkname.length-3,linkname.length)=='pdf' ||
			linkname.substring(linkname.length-3,linkname.length)=='PDF')
		{
			fileslink += '<a href='+download_str+' rel="PDF" title="'+linkname+'" caption="'+escape(caption)+'" author="None">';
			fileslink += '<img src="images/blockphoto/File-PDF.png" alt="'+linkname+'" class="link_img" style="opacity:0.5;" /><br>';
		}
		else if(myImageCheck._fileType(linkname)==='media')
		{
			var subtype = linkname.substring(linkname.lastIndexOf('.')+1,linkname.length);
			if(myImageCheck.options.mimeTypes[subtype.toLowerCase()].substring(0,6)=="audio/")
				var params=' params="lightwindow_width=400,lightwindow_height=200"';
			else
				var params=' params="lightwindow_width=800,lightwindow_height=600"';
				
			fileslink += '<a href="'+REMOTE_URL.GetFilePath+GVar.AllPath+'&filename='+linkname+'" class="lightwindow" rel="Media" title="'+linkname+'" caption="'+escape(caption)+'"'+params+' author="None">';
			fileslink += '<img src="images/blockphoto/File-Video.png" alt="'+linkname+'" class="link_img" /><br>';
		}
		else if(myImageCheck._fileType(linkname)==='image')
		{
			fileslink += '<a href="'+REMOTE_URL.GetFilePath+GVar.AllPath+'&filename='+linkname+'" class="lightwindow" rel="Image" title="'+linkname+'" caption="'+escape(caption)+'" author="None">';
			fileslink += '<img src="'+REMOTE_URL.GetThumbPath+GVar.AllPath+'&type=small&filename='+linkname+'" alt="'+linkname+'" class="link_img" /><br>';
		}
		else
		{
			fileslink += '<a href='+download_str+' rel="Exteral" title="'+linkname+'" caption="'+escape(caption)+'" author="None">';
			fileslink += '<img src="images/blockphoto/File-Generic.png" alt="'+linkname+'" class="link_img" style="opacity:0.5;" /><br>';
		}
		fileslink += name_string+'</a>';
		return fileslink;
	},
	_createFileLink: function(linkname, obj) {
		var myImageCheck = new lightwindow();
		if(myImageCheck._fileType(linkname)==='image')
		{
			var linkstr = '["'+linkname+'"]';
			var caption = 'Download: <a href="'+REMOTE_URL.DownloadFilesPath+GVar.AllPath+'&filenames='+encodeURIComponent(linkstr)+'">'+linkname+'</a>';
			// escape(caption) here, and when show(innerHTML) should be unescape(caption).
			var imglink = '<a href="'+REMOTE_URL.GetFilePath+GVar.AllPath+'&filename='+linkname+'" class="lightwindow" rel="'+GVar.PathAry[GVar.PathAry.length-1]+'" title="'+linkname+'" caption="'+escape(caption)+'" author="None">'+linkname+'</a>';
			return imglink;
		}
		else if(myImageCheck._fileType(linkname)==='media')
		{
			var params=' params="lightwindow_width=800,lightwindow_height=600"';
				
			var linkstr = '["'+linkname+'"]';
			var caption = 'Download: <a href="'+REMOTE_URL.DownloadFilesPath+GVar.AllPath+'&filenames='+encodeURIComponent(linkstr)+'">'+linkname+'</a>';
			// escape(caption) here, and when show(innerHTML) should be unescape(caption).
			var medialink = '<a href="'+REMOTE_URL.GetFilePath+GVar.AllPath+'&filename='+linkname+'" class="lightwindow page-options" caption="'+escape(caption)+'"'+params+'>'+linkname+'</a>';
			return medialink;
		}
		else
		{
			var linkstr = '["'+linkname+'"]';
			// escape(caption) here, and when show(innerHTML) should be unescape(caption).
			var externallink = '<a href="'+REMOTE_URL.DownloadFilesPath+GVar.AllPath+'&filenames='+encodeURIComponent(linkstr)+'">'+linkname+'</a>';
			return externallink;
		}
		return null;
	},
	preAddFolder: function() {
		OBJ("adddir_screen").style.display = "";
		OBJ("screen").style.display = "none";
		OBJ("adddir_path").innerHTML = "Current Path:"+GVar.AllPath;
		OBJ("addfolder").value = "";
	},
	AddFolder: function() {
		var folder_name = OBJ("addfolder").value;
		if(folder_name!="")
		{
			API.GetJsonR(GVar.PathAry, GVar.AllPath, REMOTE_URL.AddDir, REMOTE_URL.AddDirPath, API._Callback_AddDir, "&dirname="+folder_name);
		}
		else
		{
			alert("Invalid name.");
		}
	},
	CancelAddFolder: function() {
		OBJ("adddir_screen").style.display = "none";
		OBJ("screen").style.display = "";
	},
	_Callback_AddDir: function(jsonText) {
		var json = eval('('+jsonText+')');
		if(json.status=="ok")
			API.Reload();
		else
			alert("Add folder error.");
			
		OBJ("adddir_screen").style.display = "none";
		OBJ("screen").style.display = "";
	},
	preUpload: function() {
		OBJ("upload_screen").style.display = "";
		OBJ("screen").style.display = "none";
		OBJ("span_upload_path").innerHTML = "Current Path:"+GVar.AllPath;
		OBJ("upload_file").value = "";
	},
	Upload: function(c, f) {
		var upload_file_name = OBJ("upload_file").value;
		if(upload_file_name.indexOf("/")<0)
			var filename = upload_file_name.substring(upload_file_name.lastIndexOf("\\")+1,upload_file_name.length);
		else
			var filename = upload_file_name.substring(upload_file_name.lastIndexOf("/")+1,upload_file_name.length);

		OBJ("upload_id").value = this.auth_name;
		OBJ("upload_volid").value = this.auth_volid;
		OBJ("upload_tok").value = this.auth_tok;
		
		/* 'path' always create with DOM method. */
		var path_text = document.createElement('input');
		path_text.setAttribute("type","hidden");
		path_text.setAttribute("name","path");
		path_text.setAttribute("value",GVar.AllPath);
		OBJ("upload_path").appendChild(path_text);
		
		return AIM.submit(c,f);
	},
	CancelUpload: function() {
		OBJ("upload_screen").style.display = "none";
		OBJ("screen").style.display = "";
	},
	_SetValue: function(id,volid,tok) {
		this.auth_name = id;
		this.auth_pwd = '';
		this.auth_volid = volid;
		this.auth_tok = tok;
	},
	_OnLoadSuccess: function() {
		lightwindowInit();
	},
	_LoginSuccess: function(sary) {
		for(var i=1;i<sary.length;i++)
		{
			if(sary[1]=="")		GVar.PathAry.push("/");
			else				GVar.PathAry.push(sary[i]);
		}
	},
	Logout: function() {
		Login.delCookie();
		Login.checkCookie();
	}
}

function UserType(type, reload)
{
	if(API.user_type!=type)
	{
		API.user_type = type;
		if(API.user_type==CurrentType.Line)
		{
			OBJ("blockcontainer").style.display = "none";
			OBJ("detailcontainer").style.display = "";
		}
		else
		{
			OBJ("blockcontainer").style.display = "";
			OBJ("detailcontainer").style.display = "none";
		}
		if(reload)	API.Reload();
	}
}

var API = new API();
var Login = new Login();
Login.checkCookie();


function LoadIMGShow()
{
	OBJ("loading_img").style.display = '';
}
function LoadIMGHide()
{
	OBJ("loading_img").style.display = 'none';
}

function NotWork()
{
	alert("Not yet work!");
}

/* Upload file start */
function startCallback() {
	// make something useful before submit (onStart)
	return true;
}

function completeCallback(response) {
	// make something useful after (onComplete)
	// response is the innerHTML of the iframe
	if(response.status=="ok")
	{
		alert("Upload file success!");
		OBJ("upload_screen").style.display = "none";
		OBJ("screen").style.display = "";
		API.Reload();
	}
	else
	{
		alert(response.errno);
	}
}
/* Upload file end */
   