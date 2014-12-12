/* position.js used to get object's position and mouse's currently position. */
/* The position may be various according to different browser. */

/* get object's X-axis absolute position */
function COMM_GetX(obj)
{
	var X = 0;
	while( obj != null )
	{
		X += obj.offsetLeft;
		obj = obj.offsetParent;
	}
	return X;
}
/* get object's Y-axis absolute position */
function COMM_GetY(obj)
{
	var Y = 0;
	while( obj != null ) 
	{
		Y += obj.offsetTop;
		obj = obj.offsetParent;
	}
	return Y;
}
/* get mouse's X-axis absolute position */
function getMouseX(e)
{
	var ev=(!e) ? window.event : e ;/* Make sure ev get windows event */
	if (ev.pageX)/* Firefox or compatible browser */
	{
		return ev.pageX;
	}
	else if(ev.clientX)/* IE or compatible browser */
	{
		return ev.clientX;
	}
	else/* old browsers. Don't know how to do. So do nothing */
	{
		return 0;
	}
}
/* get mouse's X-axis absolute position */
function getMouseY(e)
{
	var ev=(!e) ? window.event : e ;/* Make sure ev get windows event */
	if (ev.pageY)/*Firefox or compatible browser*/
	{
		return ev.pageY;
	}
	else if(ev.clientY)/*IE or compatible browser*/
	{
		return ev.clientY;
	}
	else/* old browsers. Don't know how to do. So do nothing */
	{
		return 0;
	}
}
/* mousecheck function is an event to represent Onblur event. */
/* We can also used Onblur event too. */
/* However, sometimes Onblur event didn't work because object's CSS leak of information. */
/* So this function can cover this event and easily to use due to its cross browser feature. */
function mousecheck(e)
{
	var X_position = COMM_GetX(OBJ("language_menu"));
	var Y_position = COMM_GetY(OBJ("language_menu"));
	var menu_w = OBJ("language_menu").offsetWidth;
	var menu_h = OBJ("language_menu").offsetHeight;
	var bound1 = X_position + menu_w;
	var bound2 = Y_position + menu_h;
	var x= getMouseX(e);
	var y= getMouseY(e);

	Y_position=Y_position-50;/* optional */

	if( !(x > X_position && x < bound1 && y > Y_position && y < bound2) )
	{
		OBJ('language_menu').style.display='none';
	}
}