function drawer()
{
	var T=this;
	T.canx=0; T.cany=0;
	T.linestring=["","","","",""];
	T.linecolor=["#000","#F00","#0F0","#00B","#AAA"];
	T.moveTo=function( x,y,C)
	{
		if ( C==null) {C=4;}
		T.linestring[C] += " M "+x+","+y;
	}
	T.lineTo=function( x,y,C)
	{
		if ( C==null) {C=4;}
		T.linestring[C] += " L "+x+","+y;
	}
	T.drawpoint=function(x,y,s,C)
	{
		if (!s) {s=2;}
		if (C==null) {C=0;}
		T.moveTo(x-s,y-s,C);
		T.lineTo(x+s,y+s,C);
		T.moveTo(x-s,y+s,C);
		T.lineTo(x+s,y-s,C);
		T.moveTo(x,y,C);
	}
	T.drawarrow=function( ax,ay,bx,by, size,C)
	{
		if ( size != null)
		{
			var vx=ay-by;
			var vy=bx-ax;
			var nsize=Math.sqrt(vx*vx+vy*vy);
			var cx=vy*size/nsize;
			var cy=vx*size/nsize;
			var bx=ax+cx;
			var by=ay-cy;
		}
		T.moveTo(ax,ay, C);
		T.lineTo(bx,by, C);
		//
		var R=0.9; var K=0.04;
		var vx=ay-by;
		var vy=bx-ax;
		var acx=vy*R+ax;
		var acy=-vx*R+ay;
		//
		T.moveTo(acx+vx*K,acy+vy*K, C)
		T.lineTo(bx,by, C);
		T.lineTo(acx-vx*K,acy-vy*K, C);
	}
	T.drawpath=function(rl)
	{
		for ( var i=0; i<5; i++)
			rl.path( T.linestring[i]).attr( {stroke: T.linecolor[i], "stroke-width": 0.5});
		T.linestring=["","","","",""];
	}
	T.fillpath=function(rl,i)
	{
		rl.path( T.linestring[i]).attr( {fill: T.linecolor[i]});
		T.linestring[i]="";
	}
}
