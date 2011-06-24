function dragger(TP,div_id)
{
	this.drag=-1;
	this.TP=TP;

	this.onMouseDown = function(e)
	{
		if (e.token=='in') {
			this.parent=e.parent;
		} else {
			var T=this.parent;
			var TP=T.TP;

			e=e?e:event;
			T.xmouse=e.clientX-T.canx+document.body.scrollLeft;
			T.ymouse=e.clientY-T.cany+document.body.scrollTop;
			T.lastxmouse=T.xmouse;
			T.lastymouse=T.ymouse;
			//
			for ( var i=0; i<TP.px.length; i++)
			{
				if (Math.abs(T.xmouse - TP.px[i]) < 12 && Math.abs(T.ymouse - TP.py[i]) < 12)
				{
					T.drag = i;
					return 1;
				}
			}
			T.drag = -1;
		}
	}

	this.onMouseUp = function(e)
	{
		if (e.token=='in') {
			this.parent=e.parent;
		} else {
			var T=this.parent;
			var TP=T.TP;

			T.drag=-1;
		}
	}
	this.onMouseMove = function(e)
	{
		if (e.token=='in') {
			this.parent=e.parent;
		} else {
			var T=this.parent;
			var TP=T.TP;

			e=e?e:event;
			T.xmouse=e.clientX-T.canx+document.body.scrollLeft;
			T.ymouse=e.clientY-T.cany+document.body.scrollTop;
			if ( T.drag!=-1)
			{
				var i = T.drag;
				TP.px[i] = T.xmouse;
				TP.py[i] = T.ymouse;
				TP.redraw();
			}
			T.lastxmouse=T.xmouse;
			T.lastymouse=T.ymouse;
		}
	}
	this.printpoints = function(TP)
	{
		xstr = "this.TP.px=[";
		ystr = "this.TP.py=[";
		for ( var i=0; i<TP.px.length; i++)
		{
			if ( i != 0)
			{
				xstr+=",";
				ystr+=",";
			}
			xstr += TP.px[i];
			ystr += TP.py[i];
		}
		xstr += "];";
		ystr += "];";
		alert(xstr+"\n"+ystr);
	}

	tardiv = document.getElementById(div_id);
	
	function getPositionLeft(This){
		var el=This;var pL=0;
		while(el){pL+=el.offsetLeft;el=el.offsetParent;}
		return pL
	}
	function getPositionTop(This){
		var el=This;var pT=0;
		while(el){pT+=el.offsetTop;el=el.offsetParent;}
		return pT
	}
	this.canx=getPositionLeft(tardiv);
	this.cany=getPositionTop(tardiv);
	
	tardiv.onmousedown	=this.onMouseDown;
	tardiv.onmouseup	=this.onMouseUp;
	tardiv.onmousemove	=this.onMouseMove;

	tardiv.onmousedown({token:'in', parent:this});
	tardiv.onmouseup({token:'in', parent:this});
	tardiv.onmousemove({token:'in', parent:this});
}
