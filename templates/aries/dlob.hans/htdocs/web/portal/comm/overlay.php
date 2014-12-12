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
var overlay_element = null; //A global var that will reference the overlay window

//initOverlay()
//Call during the page init to set up the overlay window.
//Takes a string as a parameter - changes overlay color
function initOverlay(overlayColor){
	var newOverlay = document.createElement("div");

	newOverlay.id = 'overlay_base';
	newOverlay.className = 'overlay_base';
	newOverlay.style.display = 'none';
	newOverlay.style.backgroundColor = overlayColor;
	/*
	#overlay_base{
		z-index:100;
		background-color:black;
		position:absolute;
		top:0px;
		left:0px;
		width:120%;
		height:2600px;
		opacity:.5;
		-moz-opacity:.5;
		-khtml-opacity:.5;
		filter:alpha(opacity=50);
		opacity:.50;
		display:none;
	}
	*/
	//Not only does this make sure that the overlay window is snug against the boarders, 
	//but IE requires elements that use filters to have a defined with and height
	newOverlay.style.width = getPageWidth() + 'px';
	newOverlay.style.height = getPageHeight() + 'px';

	overlay_element = newOverlay;
	document.body.insertBefore(overlay_element, document.body.firstChild);
	addEvent('resize',overlay_resize,window);
}

function overlay_resize(e){
	overlay_element.style.width = getPageWidth() + 'px';
	overlay_element.style.height = getPageHeight() + 'px';
}

//enableOverlay()
//Turns the overlay window on
function enableOverlay(e){
	overlay_element.style.display="block";
}

//disalbeOverlay()
//Turns the overlay window off
function disalbeOverlay(e){
	overlay_element.style.display="none";
}

function getPageWidth(){
	var ourWidth = 0;
	if(document.body.scrollWidth > document.body.offsetWidth) //apparently not true in IE Mac
		ourWidth = document.body.scrollWidth;
	else 
		ourWidth = document.body.offsetWidth;

	var ourWindow = 0;
	if(self.innerWidth) //FF
		ourWindow = self.innerWidth;
	else if(document.documentElement.clientWidth) //not sure why I should keep this seperate, but for IE
		ourWindow = document.documentElement.clientWidth;
	else //IE
		ourWindow = document.body.clientWidth;

	if(ourWidth < ourWindow) //for small pages
		return ourWindow
	else
		return ourWidth;
}

function getPageHeight(){
	var ourHeight = 0;
	if(document.body.scrollHeight > document.body.offsetHeight) //apparently not true in IE Mac
		ourHeight = document.body.scrollHeight;
	else 
		ourHeight = document.body.offsetHeight;

	var ourWindow = 0;
	if(self.innerHeight) //FF
		ourWindow = self.innerHeight;
	else if(document.documentElement.clientHeight) //not sure why I should keep this seperate, but for IE
		ourWindow = document.documentElement.clientHeight;
	else //IE
		ourWindow = document.body.clientHeight;

	if(ourHeight < ourWindow) //for small pages
		return ourWindow;
	else
		return ourHeight;
}
</script>