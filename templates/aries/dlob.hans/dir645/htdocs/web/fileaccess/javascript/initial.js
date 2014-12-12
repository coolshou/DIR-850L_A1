
var DEFAULT_URL = {
	HOST_IP:'http://192.168.0.1',
	GET_PATH_HEADER:'/dws/api/',
	GET_USER_INFO:''
};

var REMOTE_URL = {
	ListDir:'ListDir',
	//ListDirPath:DEFAULT_URL.HOST_IP+DEFAULT_URL.GET_PATH_HEADER+'ListDir'+DEFAULT_URL.GET_USER_INFO,
	ListFile:'ListFile',
	//ListFilePath:DEFAULT_URL.HOST_IP+DEFAULT_URL.GET_PATH_HEADER+'ListFile'+DEFAULT_URL.GET_USER_INFO,
	GetThumb:'GetThumb',
	//GetThumbPath:DEFAULT_URL.HOST_IP+DEFAULT_URL.GET_PATH_HEADER+'GetThumb'+DEFAULT_URL.GET_USER_INFO,
	GetFile:'GetFile',
	//GetFilePath:DEFAULT_URL.HOST_IP+DEFAULT_URL.GET_PATH_HEADER+'GetFile'+DEFAULT_URL.GET_USER_INFO,
	DownloadFiles:'DownloadFiles',
	//DownloadFilesPath:DEFAULT_URL.HOST_IP+DEFAULT_URL.GET_PATH_HEADER+'DownloadFiles'+DEFAULT_URL.GET_USER_INFO
	AddDir:'AddDir',
	//AddDirPath:''
	UploadFile:'UploadFile'
	//UploadFilePath:''
};

// called from login success.
function Init_Default_Value(path)
{
	DEFAULT_URL.HOST_IP = "http://"+top.location.hostname;
	DEFAULT_URL.GET_USER_INFO = path;
	REMOTE_URL.ListDirPath = DEFAULT_URL.HOST_IP+DEFAULT_URL.GET_PATH_HEADER+REMOTE_URL.ListDir+DEFAULT_URL.GET_USER_INFO;
	REMOTE_URL.ListFilePath = DEFAULT_URL.HOST_IP+DEFAULT_URL.GET_PATH_HEADER+REMOTE_URL.ListFile+DEFAULT_URL.GET_USER_INFO;
	REMOTE_URL.GetThumbPath = DEFAULT_URL.HOST_IP+DEFAULT_URL.GET_PATH_HEADER+REMOTE_URL.GetThumb+DEFAULT_URL.GET_USER_INFO;
	REMOTE_URL.GetFilePath = DEFAULT_URL.HOST_IP+DEFAULT_URL.GET_PATH_HEADER+REMOTE_URL.GetFile+DEFAULT_URL.GET_USER_INFO;
	REMOTE_URL.DownloadFilesPath = DEFAULT_URL.HOST_IP+DEFAULT_URL.GET_PATH_HEADER+REMOTE_URL.DownloadFiles+DEFAULT_URL.GET_USER_INFO;
	REMOTE_URL.AddDirPath = DEFAULT_URL.HOST_IP+DEFAULT_URL.GET_PATH_HEADER+REMOTE_URL.AddDir+DEFAULT_URL.GET_USER_INFO;
	REMOTE_URL.UploadFilePath = DEFAULT_URL.HOST_IP+DEFAULT_URL.GET_PATH_HEADER+REMOTE_URL.UploadFile;
	GVar.PathAry = [];
}

var DEF_CONFIG = {
	ROOT : 'root',
	PHOTO : 'photo',
	MUSIC : 'music',
	VIDEO : 'video',
	OTHER : 'other'
};

// init golbal value
var GVar = {
	PathAry : new Array(),
	AllPath : null
};

var CurrentType = { //1:line detail, 2:block(icon) detail
	Line : 1,
	Icon : 2
};

//http://192.168.0.1/ws/api/GetFile?id=teresa&volid=TERESA&path=/aa/Video&tok=aaa:444&filename=gizmo.ogv
var Browser_IE = false;
if(navigator.appName=="Microsoft Internet Explorer")
	Browser_IE = true;
else
	Browser_IE = false;

function COMM_GetObj(id)
{
	if		(document.getElementById)	return document.getElementById(id);//.style;
	else if	(document.all)				return document.all[id].style;
	else if	(document.layers)			return document.layers[id];
	else								return false;
}
function COMM_AddObj(id)
{
	return document.createElement(id);
}
function COMM_SetObj(obj, name, value)
{
	obj.setAttribute(name, value);
	return;
}
