function XmlDocument(xml)
{
	this.XDoc = xml;
	this.AnchorNode = null;
}
XmlDocument.prototype =
{
	Serialize : function ()
	{
		var xmlString;
		if (window.ActiveXObject) xmlString = this.XDoc.xml;
		else xmlString = (new XMLSerializer()).serializeToString(this.XDoc);
		return xmlString;
	},
	dbgdump : function ()
	{
		var ow = window.open();
		ow.document.open("content-type: text/xml");
		ow.document.write(this.Serialize());
	}
}

function HTTPClient(){}
HTTPClient.prototype =
{
	debug: false,
	__httpRequest : null,
	requestMethod : "POST",
	requestAsyn : true,
	returnXml : true,
	__header : null,
	onSend : null,
	onCallback : null,
	onError : function (msg)
	{
		if (!msg) throw (msg);
	},
	__callback : function()
	{
		if(!this.__httpRequest)
		{
			this.onError("Error : Request return error("+ this.__httpRequest.status +").");
		}
		else
		{
			if (this.__httpRequest.readyState == 2)
			{
				if (this.onSend) this.onSend();
			}
			else if (this.__httpRequest.readyState == 4)
			{
				if (this.__httpRequest.status == 200)
				{
					if (this.onCallback)
					{
						if (this.returnXml)
						{
							var xdoc = new XmlDocument(this.__httpRequest.responseXML);
							if (xdoc != null)
							{
								if (this.debug) xdoc.dbgdump();
								this.onCallback(xdoc);
							}
							else this.onError("Error : unable to create XmlDocument().");
						}
						else this.onCallback(this.__httpRequest.responseText);
					}
				}
				else
				{
					this.onError("Error : Request return error("+ this.__httpRequest.status +").");
				}
			}
		}
	},
	createRequest : function()
	{
		try
		{
			// For Mazilla or Safari or IE7
			this.__httpRequest = new XMLHttpRequest();
		}
		catch (e)
		{
			var __XMLHTTPS = new Array( "MSXML2.XMLHTTP.5.0",
										"MSXML2.XMLHTTP.4.0",
										"MSXML2.XMLHTTP.3.0",
										"MSXML2.XMLHTTP",
										"Microsoft.XMLHTTP" );
			var __Success = false;
			for (var i = 0; i < __XMLHTTPS.length && __Success == false; i+=1)
			{
				try
				{
					this.__httpRequest = new ActiveXObject(__XMLHTTPS[i]);
					__Success = true;
				}
				catch (e) { }
				if (!__Success)
				{
					this.onError("Browser do not support Ajax.");
				}
			}
		}
	},
	sendRequest : function(requestUrl, payload)
	{
		if (!this.__httpRequest) this.createRequest();
		var self = this;
		this.__httpRequest.onreadystatechange = function() {self.__callback();}
		if (!requestUrl)
		{
			this.onError("Error : Invalid request URL.");
			return;
		}
		this.__httpRequest.open(this.requestMethod, requestUrl, this.requestAsyn);
		if (this.__header)
		{
			for (var i = 0; i < this.__header.length; i+=1)
			{
				if (this.__header[i].value != "")
					this.__httpRequest.setRequestHeader(this.__header[i].name, this.__header[i].value);
			}
		}
		if (this.requestMethod == "GET" || this.requestMethod == "get")
			this.__httpRequest.send(null);
		else
		{
			if (!payload)
			{
				this.onError("Error : Invalid payload for POST.");
				return;
			}
			this.__httpRequest.send(payload);
		}
	},
	getResponseHeader : function(header)
	{
		if (!header)
		{
			this.onError("Error : You must assign a header name to get.");
			return "";
		}
		if (!this.__httpRequest)
		{
			this.onError("Error : The HTTP request object is not exist.");
			return "";
		}
		return this.__httpRequest.getResponseHeader(header);
	},
	getAllResponseHeaders : function()
	{
		if (this.__httpRequest) return this.__httpRequest.getAllResponseHeaders();
		else this.onError( "Error : The HTTP request object is not exist." );
	},
	setHeader : function(header, value)
	{
		if (header && value)
		{
			if (!this.__header) this.__header = new Array();
			var tmpHeader = new Object();
			tmpHeader.name = header;
			tmpHeader.value = value;
			this.__header[ this.__header.length ] = tmpHeader;
		}
	},
	clearHeader : function (header)
	{
		if (!this.__header) return;
		if (!header) return;
		for (var i = 0; i < this.__header.length; i+=1)
		{
			if (this.__header[i].name == header)
			{
				this.__header.value = "";
				return;
			}
		}
	},
	clearAllHeaders : function()
	{
		if (!this.__header) return;
		this.__header = null;
	},
	release : function()
	{
		this.__httpRequest = null;
		this.requestMethod = "POST";
		this.requestAsyn = true;
		this.returnXml = true;
		this.__header = null;
		this.onCallback = null;
		this.onSend = null;
	}
};

var AJAX_OBJ = new Array();

function GetAjaxObj(name)
{
	var i=0;
	var ajax_num = AJAX_OBJ.length;
	if (ajax_num > 0)
	{
		for (i=0; i<ajax_num; i+=1)
		{
			if (AJAX_OBJ[i][0] == name)
			{
				return AJAX_OBJ[i][1];
			}
		}
	}
	AJAX_OBJ[ajax_num] = new Array();
	AJAX_OBJ[ajax_num][0] = name;
	AJAX_OBJ[ajax_num][1] = new HTTPClient();

	return AJAX_OBJ[ajax_num][1];
}

function OnunloadAJAX()
{
	var i;
	for (i=0; i<AJAX_OBJ.length; i+=1)
	{
		AJAX_OBJ[i][0]="";
		AJAX_OBJ[i][1].release();
		delete AJAX_OBJ[i][1];
		delete AJAX_OBJ[i];
	}
}
