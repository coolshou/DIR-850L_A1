<script>
/******************************************************************************
SSLBRIDGE:Remotely access Network Neighborhood using just a browser.
http://www.epiware.com
Copyright (C) 2006 Patrick Waddingham

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

Epiware, Inc., hereby disclaims all copyright
interest in the program `SSLBridge' written
by Patrick Waddingham.

21 August 2006
James Kern, President of Epiware
*****************************************************************************/
	var window_relative_ex_value = 0;
	var window_relative_why_value = 0;
	var window_is_moving = false;
	var window_made_count = 0;
	var window_currently_moving = null;
	var window_resize_box = null;

	var window_z_index_top_modifier = 0;
	var window_float_z_index = 9001;
	var window_normal_z_index = 101;

	var window_parameters = new Array();
	window_parameters["borderColor"] = 'black';
	window_parameters["borderWidth"]='2';
	window_parameters["mainBackgroundColor"]='white';
	window_parameters["topBackgroundColor"]='#999999';
	window_parameters["topTextColor"]='white';
	window_parameters["closeImage"]='';
	window_parameters["resizeWindow"] = "yes";
	window_parameters["resizeImage"] = '';
	window_parameters["overlay"] = 'yes';

	var window_singlet = false;

	function window_drag_start(e){
		var evt = getEvent(e);
		var windowID = parseInt(evt.target.id);
		var re = new RegExp("[0-9]our_dragable_window_close");
		if(re.test(evt.target.id)){
			return;
		}
		window_currently_moving = document.getElementById(windowID+"our_dragable_window");
		window_currently_moving.style.zIndex = window_float_z_index + window_z_index_top_modifier;
		document.getElementById(windowID+"our_dragable_window_content").style.visibility="hidden";

		//calculate window_relative_ex_value and window_relative_why_value
		window_relative_ex_value = parseInt(window_currently_moving.style.left);
		window_relative_why_value = parseInt(window_currently_moving.style.top);

		if(evt.target.attachEvent){
			window_relative_ex_value = window_relative_ex_value - evt.clientX;
			window_relative_why_value = window_relative_why_value - evt.clientY;
		} else {
			window_relative_ex_value = window_relative_ex_value - evt.pageX;
			window_relative_why_value = window_relative_why_value - evt.pageY;
		}
		
		if(window.attachEvent){
			addEvent('mousemove', window_drag_while, document.body);
			addEvent('mouseup', window_drag_end, document.body);
		} else{
			addEvent('mousemove', window_drag_while, window);
			addEvent('mouseup', window_drag_end, window);
		}
		window_is_moving = true;
	}

	function window_drag_while(e){
		var evt = getEvent(e);
		//reposition the div to where we currently are
		if(evt.target.attachEvent){
			window_currently_moving.style.top = (evt.clientY + window_relative_why_value) + 'px';
			window_currently_moving.style.left = (evt.clientX + window_relative_ex_value)+ 'px';
		} else {
			window_currently_moving.style.top = (evt.pageY + window_relative_why_value) + 'px';
			window_currently_moving.style.left = (evt.pageX + window_relative_ex_value)+ 'px';
		}
	}

	function window_drag_end(e){
		//deactivate the listeners we created
		var evt = getEvent(e);
		//alert(evt.target.id);
		try{
			document.getElementById(parseInt(evt.target.id)+"our_dragable_window_content").style.visibility="visible";
		} 
		catch(e){
			return;
		}
		if(window.attachEvent){
			removeEvent('mousemove', window_drag_while, document.body);
			removeEvent('mouseup', window_drag_end, document.body);
		} else {
			removeEvent('mousemove', window_drag_while, window);
			removeEvent('mouseup', window_drag_end, window);
		}
		window_z_index_top_modifier++;
		window_currently_moving.style.zIndex = window_normal_z_index + window_made_count + window_z_index_top_modifier;
		window_is_moving = false;
	}

	function window_focus(e){

		var evt = getEvent(e);
		window_z_index_top_modifier++;
		document.getElementById(parseInt(evt.target.id)+"our_dragable_window").style.zIndex = window_normal_z_index + window_made_count + window_z_index_top_modifier;
	}

	function window_remove_view_all(){
		//document.getElementById(window_open_list[ourIterator].style.visibility="hidden";
	}

	function window_restore_view_all(){
		//document.getElementById(window_open_list[ourIterator].style.visibility="visible";
	}

	//Singlet Functions
	function window_restore_singlet(startX, startY, width, height, url, title, whereTo){
		if(window_parameters["overlay"] == 'yes'){
			enableOverlay();
		}
		var ourDiv = document.getElementById((window_made_count - 1)+"our_dragable_window");	

		//adjust the title
		var topText = document.getElementById((window_made_count - 1) + "our_dragable_window_text");
		while(topText.lastChild != null)
			topText.removeChild(topText.lastChild);
		topText.appendChild(document.createTextNode(title));

		//change the url
		var contentBar = document.getElementById((window_made_count - 1) + "our_dragable_window_content");
		contentBar.src = url;

		//adjust the size
		
		ourDiv.style.width = width + 'px';
		ourDiv.style.height = height + 'px';
		document.getElementById((window_made_count - 1) + "our_dragable_window_bar").width = width - 5*(window_parameters["borderWidth"])+'px';  //Odd multiplier *shrug*  It works, go with it
		contentBar.style.width=(width)+"px";
	
		contentBar.style.width=(width)+"px";
		var window_working_on = document.getElementById((window_made_count - 1)+"our_dragable_window");
		var window_working_on_content = document.getElementById((window_made_count - 1)+"our_dragable_window_content");
		var window_working_on_top_bar = document.getElementById((window_made_count - 1)+"our_dragable_window_bar");

		window_working_on.style.width = width;
		window_working_on.style.height = height;

		window_working_on_content.style.height=(height-40-(2*window_parameters["borderWidth"]))+"px";
		window_working_on_content.style.width=width+"px";

		//IE bug for listeners makes me have to adjust this - doesn't hurt doing it to FF as well
		window_working_on_top_bar.style.width = width - 5*(window_parameters["borderWidth"])+'px';  //Odd multiplier *shrug*  It works, go with it


		//adjust the start position
		if(startX < 0){
			startX = (getPageWidth()/2) - (width/2);
		} 
		if(startY < 0){
			startY = (getPageHeight()/2) - (height/2);
		}
		ourDiv.style.left = startX + 'px';
		ourDiv.style.top = startY + 'px';
		contentBar.style.visibility = 'visible';
	}

	function window_destroy_singlet(useblank){
		var browserName=navigator.appName;	
		if (browserName!="Netscape" || useblank){ 
			var contentBar = document.getElementById((window_made_count - 1) + "our_dragable_window_content");
			contentBar.src = './blank.html';
		}
		if(window_parameters["overlay"] == 'yes'){
			disalbeOverlay();
		}
		var ourDiv = document.getElementById((window_made_count - 1)+"our_dragable_window");	
		//teresa, set style to hidden 
		/*ourDiv.style.left = "-10000px";
		ourDiv.style.height = "-10000px";*/
		ourDiv.style.visibility = "hidden";
		contentBar.style.visibility = 'hidden';
	}
	//End Singlet Functions

	function window_destroy_window(e){
		if(window_singlet){
			window_destroy_singlet(true);
			return;
		}
		if(window_parameters["overlay"] == 'yes'){
			disalbeOverlay();
		}
		var evt = getEvent(e);
		var ourDiv = document.getElementById(parseInt(evt.target.id)+"our_dragable_window");
		var ourParent = ourDiv.parentNode;
		ourParent.removeChild(ourDiv);
	}

	function window_destroy_target_window(id, useblank){
		if(window_singlet){
			window_destroy_singlet(useblank);
			return;
		}
		if(window_parameters["overlay"] == 'yes'){
			disalbeOverlay();
		}
		var ourDiv = document.getElementById(id);
		var ourParent = ourDiv.parentNode;
		ourParent.removeChild(ourDiv);
	}

	function window_make_new(startX, startY, width, height, url, title, whereTo){
		if(window_singlet && window_made_count > 0){
			window_restore_singlet(startX, startY, width, height, url, title, whereTo);
			return;
		}
		var ourCount = window_made_count;
		window_made_count++;

		if(startX < 0){
			startX = (getPageWidth()/2) - (width/2);
		} 

		if(startY < 0){
			startY = (getPageHeight()/2) - (height/2);
		}

		if(window.attachEvent)
		{//IE
			top_width = 7;
			main_width = 10;
		}
		else
		{
			top_width = 0;
			main_width = 0;
		}	
		var newWindow = document.createElement("div");
		newWindow.id = ourCount + "our_dragable_window";
		newWindow.className="our_dragable_window";
		newWindow.style.position = "absolute";
		newWindow.style.top = startY + "px";
		newWindow.style.left = startX + "px";
		newWindow.style.width = width + "px";
		newWindow.style.height = height + "px";
		newWindow.style.borderColor = window_parameters["borderColor"];
		newWindow.style.borderWidth = window_parameters["borderWidth"]+"px";
		//newWindow.style.borderStyle = 'solid';
		if(window_parameters["mainBackgroundColor"]!='' && window_parameters["mainBackgroundColor"]!='none')
			newWindow.style.backgroundColor = window_parameters["mainBackgroundColor"];
		if(window.attachEvent)
			newWindow.setAttribute("unselectable","on");
		if(window.attachEvent)
			newWindow.setAttribute("UNSELECTABLE","on");

		var topBar = document.createElement("div");
		topBar.id = ourCount + "our_dragable_window_bar";
		if(window.attachEvent)
			topBar.className = "our_dragable_window_bar_ie";
		else
			topBar.className = "our_dragable_window_bar_moz";
		topBar.style.backgroundColor = window_parameters["topBackgroundColor"];
		topBar.style.borderBottom="solid "+window_parameters["borderColor"]+" "+window_parameters["borderWidth"]+"px";
		topBar.style.width = width - 5*(window_parameters["borderWidth"])+'px';  //Odd multiplier *shrug*  It works, go with it
		if(window.attachEvent)
			topBar.setAttribute("unselectable","on");
		if(window.attachEvent)
			topBar.setAttribute("UNSELECTABLE","on");

		var topText = document.createElement("div");
		topText.id=ourCount+"our_dragable_window_text";
		topText.className="our_dragable_window_text";
		topText.style.color=window_parameters["topTextColor"];
		if(typeof(title)!='undefined' && title.length > 0){
			topText.appendChild(document.createTextNode(title));
		}
		topText.style.cursor="default";
		if(window.attachEvent)
			topText.setAttribute("unselectable","on");
		if(window.attachEvent)
			topText.setAttribute("UNSELECTABLE","on");

		var rightOptions = document.createElement("span");
		rightOptions.id=ourCount+"our_dragable_window_options";
		rightOptions.className = "our_dragable_window_options";
		if(window.attachEvent)
			rightOptions.setAttribute("unselectable","on");
		if(window.attachEvent)
			rightOptions.setAttribute("UNSELECTABLE","on");

		var closeWindow = document.createElement("span");
		closeWindow.id=ourCount+"our_dragable_window_close";
		closeWindow.className = "our_dragable_window_close";
		if(window.attachEvent)
			closeWindow.setAttribute("unselectable","on");
		if(window.attachEvent)
			closeWindow.setAttribute("UNSELECTABLE","on");


		if(window_parameters["closeImage"]!=''){
			if(window_parameters["closeImage"].indexOf('.png')>=0 && window.attachEvent){
				closeWindow.style.filter="progid:DXImageTransform.Microsoft.AlphaImageLoader(src='"+window_parameters["closeImage"]+"', sizingMethod=scale)";
				closeWindow.style.width="19px";
				closeWindow.style.width="19px";
			} else {
				var ourImg = document.createElement("img");
				ourImg.id = ourCount+"our_dragable_window_close_image";
				ourImg.src=	window_parameters["closeImage"];
				ourImg.style.width='19px';
				ourImg.style.height='19px';
				if(window.attachEvent)
					ourImg.setAttribute("unselectable","on");
				if(window.attachEvent)
					ourImg.setAttribute("UNSELECTABLE","on");

				closeWindow.appendChild(ourImg);
			}
		} else {
			closeWindow.style.backgroundColor = "red";
			closeWindow.style.width='19px';
			closeWindow.style.height='19px';
			closeWindow.style.color="white";
			closeWindow.style.paddingLeft="4px";
			closeWindow.style.paddingRight="5px";

			closeWindow.appendChild(document.createTextNode("X"));
		}

		var theBar = document.createElement("div");
		theBar.id=ourCount+"our_dragable_window_clear_bar";
		theBar.className="our_dragable_window_clear_bar";
		if(window.attachEvent)
			theBar.setAttribute("unselectable","on");
		if(window.attachEvent)
			theBar.setAttribute("UNSELECTABLE","on");

		rightOptions.appendChild(closeWindow);
		topBar.appendChild(topText);
		topBar.appendChild(rightOptions);
		topBar.appendChild(theBar);


		var contentBar = document.createElement("iframe");
		contentBar.frameBorder="0";
		contentBar.id = ourCount + "our_dragable_window_content";
		contentBar.className = "our_dragable_window_content";
		contentBar.src = url;
		contentBar.style.height=(height-40-(2*window_parameters["borderWidth"]))+"px";

		contentBar.style.width=(width-main_width)+"px";

		if(window.attachEvent)
			contentBar.setAttribute("frameborder","0");
		if(window.attachEvent)
			contentBar.setAttribute("unselectable","on");
		if(window.attachEvent)
			contentBar.setAttribute("UNSELECTABLE","on");


		if(window_parameters["resizeWindow"]!='' && window_parameters["resizeWindow"]!= 'no'){
			var bottomBar = document.createElement("div");
			bottomBar.style.width="100%";
			bottomBar.id =  ourCount+"our_dragable_window_bottom_bar";
			bottomBar.style.backgroundColor = window_parameters["topBackgroundColor"];
			bottomBar.style.borderTop="solid "+window_parameters["borderColor"]+" "+window_parameters["borderWidth"]+"px";
			if(window.attachEvent)
				bottomBar.setAttribute("unselectable","on");
			if(window.attachEvent)
				bottomBar.setAttribute("UNSELECTABLE","on");

			var resizeMe = document.createElement("span");
			resizeMe.id=ourCount+"our_dragable_window_resize";
			resizeMe.className="our_dragable_window_resize";
			resizeMe.style.color=window_parameters["topTextColor"];
			resizeMe.style.backgroundColor=window_parameters["topBackgroundColor"];
			if(window.attachEvent)
				resizeMe.setAttribute("unselectable","on");
			if(window_parameters["resizeImage"]==''){
				resizeMe.style.textAlign="right";
				resizeMe.appendChild(document.createTextNode(".:"));
				if(window.attachEvent)
					resizeMe.setAttribute("unselectable","on");
				if(window.attachEvent)
					resizeMe.setAttribute("UNSELECTABLE","on");
				resizeMe.style.paddingRight="3px";
				resizeMe.style.fontWeight = "bold";
			} else {
				if(window_parameters["resizeImage"].indexOf('.png')>=0 && window.attachEvent){
					resizeMe.style.filter="progid:DXImageTransform.Microsoft.AlphaImageLoader(src='"+window_parameters["resizeImage"]+"', sizingMethod=scale)";
					resizeMe.style.width="19px";
					resizeMe.style.height="19px";
				} else {
					var ourImg = document.createElement("img");
					ourImg.id = ourCount+"our_dragable_window_close_image";
					ourImg.src=	window_parameters["resizeImage"];
					ourImg.style.width='19px';
					ourImg.style.height='19px';

					resizeMe.appendChild(ourImg);
				}				
			}

			bottomBar.appendChild(resizeMe);
			var theBar2 = document.createElement("div");
			theBar2.id=ourCount+"our_dragable_window_clear_bar2";
			theBar2.className="our_dragable_window_clear_bar";
			if(window.attachEvent)
				theBar2.setAttribute("unselectable","on");
			if(window.attachEvent)
				theBar2.setAttribute("UNSELECTABLE","on");
			bottomBar.appendChild(theBar2);
		}
	
		newWindow.appendChild(topBar);
		newWindow.appendChild(contentBar);
		if(window_parameters["resizeWindow"]!='' && window_parameters["resizeWindow"]!= 'no'){
			newWindow.appendChild(bottomBar);
			//addEvent('mousedown',window_resize_start,resizeMe);//+++Teresa, bc can't control windows size
		}

		if(typeof(whereTo)=='undefined')
			whereTo = document.body;

		whereTo.appendChild(newWindow);
		addEvent('mousedown',window_drag_start,topBar);
		addEvent('mousedown',window_focus,contentBar);
		addEvent('mousedown',window_destroy_window,closeWindow);

		newWindow.style.zIndex = window_normal_z_index + window_made_count;

		if(window_parameters["overlay"] == 'yes'){
			enableOverlay();
		}

		return newWindow.id;
	}

	function window_resize_start(e){
		var evt = getEvent(e);
		window_resize_box = document.createElement("div");
		window_resize_box.id = parseInt(evt.target.id)+"our_dragable_window_resize_box";
		window_resize_box.className = "our_dragable_window_resize_box";

		var window_working_on = document.getElementById(parseInt(evt.target.id)+"our_dragable_window");

		window_resize_box.style.top = window_working_on.style.top;
		window_resize_box.style.left = window_working_on.style.left;
		window_resize_box.style.width = window_working_on.style.width;
		window_resize_box.style.height = window_working_on.style.height;
		window_resize_box.style.zIndex = window_working_on.style.zIndex;

		document.body.appendChild(window_resize_box);

		if(window.attachEvent){
			addEvent('mousemove', window_resize_whlie, document.body);
			addEvent('mouseup', window_resize_end, document.body);
		} else{
			addEvent('mousemove', window_resize_whlie, window);
			addEvent('mouseup', window_resize_end, window);
		}
		document.body.style.cursor = 'move';
	}

	function window_resize_whlie(e){
		var evt = getEvent(e);
		
		if(evt.target.attachEvent){
			window_resize_box.style.width = (evt.clientX - parseInt(window_resize_box.style.left)) + 'px';
			window_resize_box.style.height = (evt.clientY - parseInt(window_resize_box.style.top)) + 'px';
		} else {
			window_resize_box.style.width = (evt.pageX - parseInt(window_resize_box.style.left)) + 'px';
			window_resize_box.style.height = (evt.pageY - parseInt(window_resize_box.style.top)) + 'px';
		}
	}

	function window_resize_end(e){
		document.body.style.cursor = 'default';

		var evt = getEvent(e);

		var window_working_on = document.getElementById(parseInt(evt.target.id)+"our_dragable_window");
		var window_working_on_content = document.getElementById(parseInt(evt.target.id)+"our_dragable_window_content");
		var window_working_on_top_bar = document.getElementById(parseInt(evt.target.id)+"our_dragable_window_bar");

		window_working_on.style.width = window_resize_box.style.width;
		window_working_on.style.height = window_resize_box.style.height;

		window_working_on_content.style.height=(parseInt(window_resize_box.style.height)-40-(2*window_parameters["borderWidth"]))+"px";
		window_working_on_content.style.width=(parseInt(window_resize_box.style.width))+"px";

		//IE bug for listeners makes me have to adjust this - doesn't hurt doing it to FF as well
		window_working_on_top_bar.style.width = parseInt(window_resize_box.style.width) - 5*(window_parameters["borderWidth"])+'px';  //Odd multiplier *shrug*  It works, go with it

		document.body.removeChild(window_resize_box);

		if(window.attachEvent){
			removeEvent('mousemove', window_resize_whlie, document.body);
			removeEvent('mouseup', window_resize_end, document.body);
		} else{
			removeEvent('mousemove', window_resize_whlie, window);
			removeEvent('mouseup', window_resize_end, window);
		}
	}

	//Configuration Functions ***  Not apart of running the window, just nice to have to modify
	function window_configuration(borderColor, borderWidth, mainBackgroundColor,topBackgroundColor,topTextColor,closeImage){
		window_parameters["borderColor"] = borderColor;
		window_parameters["borderWidth"]=borderWidth;
		window_parameters["mainBackgroundColor"]=mainBackgroundColor;
		window_parameters["topBackgroundColor"]=topBackgroundColor;
		window_parameters["closeImage"]=closeImage;
		window_parameters["topTextColor"]=topTextColor
	}

	function window_change_border_color(color){
		window_parameters["borderColor"] = color;
	}

	function window_change_border_width(width){
		window_parameters["borderWidth"]=width;
	}

	function window_change_background_color(color){
		window_parameters["mainBackgroundColor"]=color;
	}

	function window_change_top_color(color){
		window_parameters["topBackgroundColor"]=color;
	}

	function window_change_text_color(color){
		window_parameters["topTextColor"]=color
	}

	function window_change_close_image(img){
		window_parameters["closeImage"]=img;
	}
</script>