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
function getEvent(e){
	var eventReturn = (e || window.event);
	//alert(eventReturn.target);
	if(!eventReturn.target)
		eventReturn.target = event.srcElement;
	if(eventReturn.target.nodeType == 3)
		eventReturn.target = eventReturn.target.parentNode.parentNode.parentNode;
	return eventReturn; //gets the event information
}

function addEvent(event, funct, elem){
	//I assume it is an IE-like browser if this fails - this isn't very 'safe'
	if(elem.addEventListener) //Netscape/FireFox
		elem.addEventListener(event,funct,true);
	else //IE
		elem.attachEvent("on"+event,funct);
}

function removeEvent(event, funct, elem){
	//I assume it is an IE-like browser if this fails - this isn't very 'safe'
	if(elem.addEventListener) //Netscape/FireFox
		elem.removeEventListener(event,funct,true);
	else //IE
		elem.detachEvent("on"+event,funct);
}
</script>