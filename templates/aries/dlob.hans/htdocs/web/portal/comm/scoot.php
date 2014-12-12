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
var scoot_speed = 1;
var scooting = -1;
var scootIntervals = new Array();
var scootElements = new Array();
var scootFail = new Array();
var scootDistance = new Array();


function scoot(elem, dir, stopDistance){
	var ourID = elem.id;
	scooting++;
	if(typeof(scootIntervals[ourID])!="undefined" || scootIntervals[elem.id]!=false){ 
		clearInterval(scootIntervals[ourID]);
	}
	scootElements[ourID] = elem;
	scootDistance[ourID] = stopDistance;
	scootIntervals[ourID] = setInterval('scoot_working('+dir+','+stopDistance+',"'+ourID+'")',5);
}

function scoot_working(dir, stopDistance, scootID){
	scootDistance[scootID]-=scoot_speed;
	var position = parseInt(scootElements[scootID].style.left);
	if(dir <=0){
		position-=scoot_speed;		
	} else 
		position+=scoot_speed;
	if(dir <=0){
		if(scootDistance[scootID]<0){
			var addMe = (-1)*scootDistance[scootID];
			//shut off interval
			scooting--;
			clearInterval(scootIntervals[scootID]);
			scootIntervals[scootID] = false;
			//set position
			position+=addMe;
		} else if(scootDistance[scootID]==0){
			//shut off interval
			scooting--;
			clearInterval(scootIntervals[scootID]);
			scootIntervals[scootID] = false;
		}
	} else {
		if(scootDistance[scootID] < stopDistance){
			var subMe = (-1)*scootDistance[scootID];
			//shut off interval
			scooting--;
			clearInterval(scootIntervals[scootID]);
			scootIntervals[scootID] = false;
			//set position
			position-=subMe;
		} else if(scootDistance[scootID] == stopDistance){
			//shut off interval
			scooting--;
			clearInterval(scootIntervals[scootID]);
			scootIntervals[scootID] = false;
		}
	}
	scootElements[scootID].style.left=position+'px';
}
</script>