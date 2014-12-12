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
var fading = -1;
var fadeElements = new Array();
var fadeIntervals = new Array();
var fadeFail = new Array();
var fadeValues = new Array();

function fade(elem, amount, start, end){
	var fadeID = elem.id;
	fading++;
	if(typeof(fadeIntervals[fadeID])!="undefined" || fadeIntervals[fadeID]){ 
		clearInterval(fadeIntervals[fadeID]);
	}
	fadeFail[fadeID] = 0;
	fadeElements[fadeID] = elem;
	fadeValues[fadeID] = start;
	var dir = 0;
	if(start < end)
		dir = 1;

	//alert("start: "+start+" end: "+end);

	fadeIntervals[fadeID] = setInterval('fade_working('+amount+','+dir+','+end+',"'+fadeID+'")',5)
}

function fade_working(amount, dir, destination, fadeID){
	var ourAmount = amount;
	var ourDest = destination;

	if(dir <= 0)
		ourAmount = amount*(-1);
	
	var ourValue = fadeValues[fadeID] + ourAmount;
	fadeValues[fadeID] = ourValue;

	if(dir <= 0){
		if(ourValue <= destination){
			ourValue = destination;
			clearInterval(fadeIntervals[fadeID]);
		}	
	} else {
		if(ourValue >= destination){
			ourValue = destination;
			clearInterval(fadeIntervals[fadeID]);
		}	
	}

	fadeElements[fadeID].style.opacity = (ourValue / 100);
    fadeElements[fadeID].style.MozOpacity = (ourValue / 100);
	fadeElements[fadeID].style.KhtmlOpacity = (ourValue / 100);
    fadeElements[fadeID].style.filter = "alpha(opacity=" + ourValue + ")"; 

	//shouldn't need anymore...
	fadeFail[fadeID]++;
	if(fadeFail[fadeID] >=101){
		clearInterval(fadeIntervals[fadeID]);
		fadeIntervals[fadeID]=false;
	}

	//alert(dir + " OurValue: " + ourValue + " destination: " + destination);
}
</script>