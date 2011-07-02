function triangle_knife_cut(p1,p2,p3,kn1,kn2,kn3)
{
	var poly = []; //the retained polygon
	var poly_cut = []; //the cut away polygon

	var std_sign = signed_area( kn1,kn2,kn3);
	var s1 = signed_area( kn1,kn2,p1)>0 == std_sign>0; //true means this point should be cut
	var s2 = signed_area( kn1,kn2,p2)>0 == std_sign>0;
	var s3 = signed_area( kn1,kn2,p3)>0 == std_sign>0;
	var sums = s1+s2+s3;
	
	if ( sums == 0)
	{	//all 3 points are retained
		poly.push(p1);
		poly.push(p2);
		poly.push(p3);
		return { poly:poly, poly_cut:poly_cut};
	}
	else if ( sums == 3)
	{	//all 3 are cut away
		poly_cut.push(p1);
		poly_cut.push(p2);
		poly_cut.push(p3);
		return { poly:poly, poly_cut:poly_cut};
	}
	else
	{
		if ( sums == 2) {
			s1 = !s1;
			s2 = !s2;
			s3 = !s3;
		}
		//
		var ip1,ip2, outp;
		if ( s1) { //here assume one point is cut away
			outp= p1;
			ip1 = p2;
			ip2 = p3;
		} else if ( s2) {
			outp= p2;
			ip1 = p1;
			ip2 = p3;
		} else if ( s3) {
			outp= p3;
			ip1 = p1;
			ip2 = p2;
		}
		
		var interP1 = intersect( kn1,kn2, ip2,outp);
		var interP2 = intersect( kn1,kn2, ip1,outp);
		//ip2 first gives a polygon
		//ip1 first gives a triangle strip
		
		poly.push(ip1);
		poly.push(ip2);
		poly.push(interP1);
		poly.push(interP2);
		
		poly_cut.push(outp);
		poly_cut.push(interP1);
		poly_cut.push(interP2);
		
		if ( sums == 1) {
			return { poly:poly, poly_cut:poly_cut};
		} else if ( sums == 2) {
			return { poly:poly_cut, poly_cut:poly};
			//^ we swap the result!
		}
	}
}
function signed_area(p1,p2,p3)
{
	var D = (p2.x-p1.x)*(p3.y-p1.y)-(p3.x-p1.x)*(p2.y-p1.y);
	return D;
}
function intersect( P1,P2,  //line 1
			P3,P4) //line 2
{ //Determine the intersection point of two line segments
	var mua,mub;
	var denom,numera,numerb;

	denom  = (P4.y-P3.y) * (P2.x-P1.x) - (P4.x-P3.x) * (P2.y-P1.y);
	numera = (P4.x-P3.x) * (P1.y-P3.y) - (P4.y-P3.y) * (P1.x-P3.x);
	numerb = (P2.x-P1.x) * (P1.y-P3.y) - (P2.y-P1.y) * (P1.x-P3.x);
	
	function negligible(M)
	{
		return -0.00000001 < M && M < 0.00000001;
	}
	
	if ( negligible(numera) && negligible(numerb) && negligible(denom)) {
		//meaning the lines coincide
		return { x: (P1.x + P2.x) * 0.5,
			y:  (P1.y + P2.y) * 0.5 };
	}

	if ( negligible(denom)) {
		//meaning lines are parallel
		return { x:0, y:0};
	}

	mua = numera / denom;
	mub = numerb / denom;

	return { x: P1.x + mua * (P2.x - P1.x),
		y:  P1.y + mua * (P2.y - P1.y) };
//http://paulbourke.net/geometry/lineline2d/
}
