namespace VASErin
{	//VASEr internal namespace

/*visual testes:
 * A. points (geometry test)
 *  1. arbitrary polyline of only 2 point
 *  2. polylines of 3 points, making arbitrary included angle
 *  3. arbitrary polyline of 4 or more points
 * B. colors
 *  1. different colors on each point
 * C. weight
 *  1. varying weight
 * D. other drawing options
 *  1. feathering
 *  2. different joint types
 *  3. different cap types
 * E. memory test
 *  1. drawing a polyline of 1000 points
 */

/*known visual bugs: ( ",," simply means "go wild" as it is too hard to describe)
 * 1.  [solved]when 2 segments are exactly at 90 degrees, the succeeding line segment is reflexed.
 * 1.1 [solved]when 2 segments are exactly at 180 degrees,,
 * 2.  [solved]when polyline is dragged, caps seem to have pseudo-podia.
 * 3.  [solved]when 2 segments are exactly horizontal/ vertical, both segments are reflexed.
 * 3.1 [solved]when a segment is exactly horizontal/ vertical, the cap disappears
 * 4.  [solved]when 2 segments make < 5 degrees,,
 * 4.1 [solved]when 2 segments make exactly 0 degree,,
 * 5.  when a segment is shorter than its own width,,
 */

/* const double vaser_actual_PPI = 96.0;
	const double vaser_standard_PPI = 111.94; //the PPI I used for calibration
 */

//critical weight to do approximation rather than real joint processing
const double cri_segment_approx=1.6;

void determine_t_r ( double w, double& t, double& R)
{
	//efficiency: can cache one set of w,t,R values
	// i.e. when a polyline is of uniform thickness, the same w is passed in repeatedly
	double f=w-static_cast<int>(w);
	
	/*   */if ( w>=0.0 && w<1.0) {
		t=0.05; R=0.768;//R=0.48+0.32*f;
	} else if ( w>=1.0 && w<2.0) {
		t=0.05+f*0.33; R=0.768+0.312*f;
	} else if ( w>=2.0 && w<3.0){
		t=0.38+f*0.58; R=1.08;
	} else if ( w>=3.0 && w<4.0){
		t=0.96+f*0.48; R=1.08;
	} else if ( w>=4.0 && w<5.0){
		t=1.44+f*0.46; R=1.08;
	} else if ( w>=5.0 && w<6.0){
		t=1.9+f*0.6; R=1.08;
	} else if ( w>=6.0){
		double ff=w-6.0;
		t=2.5+ff*0.50; R=1.08;
	}
	
	/* //PPI correction
	double PPI_correction = vaser_standard_PPI / vaser_actual_PPI;
	const double up_bound = 1.6; //max value of w to receive correction
	const double start_falloff = 1.0;
	if ( w>0.0 && w<up_bound)
	{	//here we gracefully apply the correction
		// so that the effect of correction diminishes starting from w=start_falloff
		//   and completely disappears when w=up_bound
		double correction = 1.0 + (PPI_correction-1.0)*(up_bound-w)/(up_bound-start_falloff);
		t *= PPI_correction;
		R *= PPI_correction;
	} */
}
float get_PLJ_round_dangle(float t, float r)
{
	float dangle;
	float sum = t+r;
	if ( sum <= 1.44f+1.08f) //w<=4.0, feathering=1.0
		dangle = 0.6f/(t+r);
	else if ( sum <= 3.25f+1.08f) //w<=6.5, feathering=1.0
		dangle = 2.8f/(t+r);
	else
		dangle = 4.2f/(t+r);
	return dangle;
}

void make_T_R_C( Point& P1, Point& P2, Point* T, Point* R, Point* C,
				double w, const polyline_opt& opt,
				double* rr, double* tt, float* dist,
				bool seg_mode=false)
{
	double t=1.0,r=0.0;
	Point DP=P2-P1;
	
	//calculate t,r
	determine_t_r( w,t,r);
	
	if ( opt.feather && !opt.no_feather_at_core && opt.feathering != 1.0)
		r *= opt.feathering;
	else if ( seg_mode)
	{
		//TODO: handle correctly for hori/vert segments in a polyline
		if ( Point::negligible(DP.x) && P1.x==(int)P1.x) {
			if ( w>0.0 && w<=1.0) {
				t=0.5; r=0.0;
				P2.x = P1.x = (int)P1.x+0.5;
			}
		} else if ( Point::negligible(DP.y) && P1.y==(int)P1.y) {
			if ( w>0.0 && w<=1.0) {
				t=0.5; r=0.0;
				P2.y = P1.y = (int)P1.y+0.5;
			}
		}
	}
	
	//output t,r
	if (tt) *tt = t;
	if (rr) *rr = r;
	
	//calculate T,R,C
	double len = DP.normalize();
	if (dist) *dist = (float)len;
	if (C) *C = DP;
	DP.perpen();
	
	if (T) *T = DP*t;
	if (R) *R = DP*r;
}

void same_side_of_line( Point& V, const Point& ref, const Point& a, const Point& b)
{
	double sign1 = Point::signed_area( a+ref,a,b);
	double sign2 = Point::signed_area( a+V,  a,b);
	if ( (sign1>=0) != (sign2>=0))
	{
		V.opposite();
	}
}

struct st_polyline
//the struct to hold info for anchor_late() to perform triangluation
{
	//for all joints
	Point vP; //vector to intersection point
	Point vR; //fading vector at sharp end
		//all vP,vR are outward
	
	//for djoint==PLJ_bevel
	Point T; //core thickness of a line
	Point R; //fading edge of a line
	Point bR; //out stepping vector, same direction as cap
	Point T1,R1; //alternate vectors, same direction as T21
		//all T,R,T1,R1 are outward
	
	//for djoint==PLJ_round
	float t,r;
	
	//for degeneration case
	bool degenT; //core degenerated
	bool degenR; //fade degenerated
	bool pre_full; //draw the preceding segment in full
	Point PT,PR;
	float pt; //parameter at intersection
	bool R_full_degen;
	
	char djoint; //determined joint
			// e.g. originally a joint is PLJ_miter. but it is smaller than critical angle, should then set djoint to PLJ_bevel
};

struct st_anchor
//the struct to hold memory for the working of anchor()
{
	Point P[3]; //point
	Color C[3]; //color
	double W[3];//weight
	
	Point cap_start, cap_end;
	st_polyline SL[3];
	vertex_array_holder vah;
};

void inner_arc( vertex_array_holder& hold, const Point& P,
		const Color& C, const Color& C2,
		float dangle, float angle1, float angle2,
		float r, float r2, bool ignor_ends,
		Point* apparent_P)	//(apparent center) center of fan
//draw the inner arc between angle1 and angle2 with dangle at each step.
// -the arc has thickness, r is the outer radius and r2 is the inner radius,
//    with color C and C2 respectively.
//    in case when inner radius r2=0.0f, it gives a pie.
// -when ignor_ends=false, the two edges of the arc lie exactly on angle1
//    and angle2. when ignor_ends=true, the two edges of the arc do not touch
//    angle1 or angle2.
// -P is the mathematical center of the arc.
// -when apparent_P points to a valid Point (apparent_P != 0), r2 is ignored,
//    apparent_P is then the apparent origin of the pie.
// -the result is pushed to hold, in form of a triangle strip
// -an inner arc is an arc which is always shorter than or equal to a half circumference
{
	const double& m_pi = vaser_pi;
	
	bool incremental=true;
	
	if ( angle2 > angle1)
	{
		if ( angle2-angle1>m_pi)
		{
			angle2=angle2-2*m_pi;
		}
	}
	else
	{
		if ( angle1-angle2>m_pi)
		{
			angle1=angle1-2*m_pi;
		}
	}
	if ( angle1>angle2)
	{
		incremental = false; //means decremental
	}
	
	if ( incremental)
	{
		if ( ignor_ends)
		{
			int i=0;
			for ( float a=angle1+dangle; a<angle2; a+=dangle, i++)
			{
				float x=cos(a);
				float y=sin(a);
				
				#define INNER_ARC_PUSH \
					hold.push( Point(P.x+x*r,P.y-y*r), C);\
					if ( !apparent_P)\
						hold.push( Point(P.x+x*r2,P.y-y*r2), C2);\
					else\
						hold.push( *apparent_P, C2);
				
				INNER_ARC_PUSH
				
				if ( i>100) {
					DEBUG("trapped in loop: inc,ig_end angle1=%.2f, angle2=%.2f, dangle=%.2f\n", angle1, angle2, dangle);
					break;
				}
			}
			//DEBUG( "steps=%d ",i); fflush(stdout);
		}
		else
		{
			int i=0;
			for ( float a=angle1; ; a+=dangle, i++)
			{
				if ( a>angle2)
					a=angle2;
				
				float x=cos(a);
				float y=sin(a);
				
				INNER_ARC_PUSH
				
				if ( a>=angle2)
					break;
				
				if ( i>100) {
					DEBUG("trapped in loop: inc,end angle1=%.2f, angle2=%.2f, dangle=%.2f\n", angle1, angle2, dangle);
					break;
				}
			}
		}
	}
	else //decremental
	{
		if ( ignor_ends)
		{
			int i=0;
			for ( float a=angle1-dangle; a>angle2; a-=dangle, i++)
			{
				float x=cos(a);
				float y=sin(a);
				
				INNER_ARC_PUSH
				
				if ( i>100) {
					DEBUG("trapped in loop: dec,ig_end angle1=%.2f, angle2=%.2f, dangle=%.2f\n", angle1, angle2, dangle);
					break;
				}
			}
		}
		else
		{
			int i=0;
			for ( float a=angle1; ; a-=dangle, i++)
			{
				if ( a<angle2)
					a=angle2;
				
				float x=cos(a);
				float y=sin(a);
				
				INNER_ARC_PUSH
				#undef INNER_ARC_PUSH
				
				if ( a<=angle2)
					break;
				
				if ( i>100) {
					DEBUG("trapped in loop: dec,end angle1=%.2f, angle2=%.2f, dangle=%.2f\n", angle1, angle2, dangle);
					break;
				}
			}
		}
	}
}
void vectors_to_arc( vertex_array_holder& hold, const Point& P,
		const Color& C, const Color& C2,
		Point A, Point B, float dangle, float r, float r2, bool ignor_ends,
		Point* apparent_P)
//triangulate an inner arc between vectors A and B,
//  A and B are position vectors relative to P
{
	const double& m_pi = vaser_pi;
	A *= 1/r;
	B *= 1/r;
	if ( A.x > 1.0-vaser_min_alw) A.x = 1.0-vaser_min_alw;
	if ( A.x <-1.0+vaser_min_alw) A.x =-1.0+vaser_min_alw;
	if ( B.x > 1.0-vaser_min_alw) B.x = 1.0-vaser_min_alw;
	if ( B.x <-1.0+vaser_min_alw) B.x =-1.0+vaser_min_alw;
	
	float angle1 = acos(A.x);
	float angle2 = acos(B.x);
	if ( A.y>0){ angle1=2*m_pi-angle1;}
	if ( B.y>0){ angle2=2*m_pi-angle2;}
	//printf( "steps=%d ",int((angle2-angle1)/den*r));
	
	inner_arc( hold, P, C,C2, dangle,angle1,angle2, r,r2, ignor_ends, apparent_P);
}

#ifdef VASER_DEBUG
void annotate( const Point& P, Color cc, int I=-1)
{
	static int i=0;
	if ( I != -1) i=I;
	
	glBegin(GL_LINES);
		glColor3f(1,0,0);
		glVertex2f(P.x-4,P.y-4);
		glVertex2f(P.x+4,P.y+4);
		glVertex2f(P.x-4,P.y+4);
		glVertex2f(P.x+4,P.y-4);
	glEnd();
	
	char str[10];
	sprintf(str,"%d",i);
	gl_font( FL_HELVETICA, 8);
	gl_draw( str,float(P.x+2),float(P.y));
	i++;
}
void annotate( const Point& P)
{
	Color cc;
	annotate(P,cc);
}
void draw_vector( const Point& P, const Point& V, const char* name)
{
	Point P2 = P+V;
	glBegin(GL_LINES);
		glColor3f(1,0,0);
		glVertex2f(P.x,P.y);
		glColor3f(1,0.9,0.9);
		glVertex2f(P2.x,P2.y);
	glEnd();
	if ( name)
	{
		glColor3f(0,0,0);
		gl_font( FL_HELVETICA, 8);
		gl_draw( name,float(P2.x+2),float(P2.y));
	}
}
void printpoint( const Point& P, const char* name)
{
	printf("%s(%.4f,%.4f) ",name,P.x,P.y);
	fflush(stdout);
}
#endif
/*
Point plus_minus( const Point& a, const Point& b, bool plus)
{
	if (plus) return a+b;
	else return a-b;
}
Point plus_minus( const Point& a, bool plus)
{
	if (plus) return a;
	else return -a;
}
bool quad_is_reflexed( const Point& P0, const Point& P1, const Point& P2, const Point& P3)
{
	//points:
	//   1------3
	//  /      /
	// 0------2
	// vector 01 parallel to 23
	
	return Point::distance_squared(P1,P3) + Point::distance_squared(P0,P2)
		> Point::distance_squared(P0,P3) + Point::distance_squared(P1,P2);
}
void push_quad_safe( vertex_array_holder& core,
		const Point& P2, const Color& cc2, bool transparent2,
		const Point& P3, const Color& cc3, bool transparent3)
{
	//push 2 points to form a quad safely(without reflex)
	Point P0 = core.get_relative_end(-2);
	Point P1 = core.get_relative_end(-1);
	
	if ( !quad_is_reflexed(P0,P1,P2,P3))
	{
		core.push(P2,cc2,transparent2);
		core.push(P3,cc3,transparent3);
	}
	else
	{
		core.push(P3,cc3,transparent3);
		core.push(P2,cc2,transparent2);
	}
}*/

#define push_quad(A0,A1,A2,A3,A4,A5,A6,A7,A8) push_quad_(__LINE__,A0,A1,A2,A3,A4,A5,A6,A7,A8)
#define push_quadf(A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12) push_quadf_(__LINE__,A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12)

void push_quad_( short line, vertex_array_holder& core,
		const Point& P0, const Point& P1, const Point& P2, const Point& P3,
		const Color& c0, const Color& c1, const Color& c2, const Color& c3)
{
	if( P0.is_zero()) DEBUG("pushed P0 (0,0) at %d\n",line);
	if( P1.is_zero()) DEBUG("pushed P1 (0,0) at %d\n",line);
	if( P2.is_zero()) DEBUG("pushed P2 (0,0) at %d\n",line);
	if( P3.is_zero()) DEBUG("pushed P3 (0,0) at %d\n",line);
	//interpret P0 to P3 as triangle strip
	core.push3( P0, P1, P2,
		c0, c1, c2);
	core.push3( P1, P2, P3,
		c1, c2, c3);
}
void push_quadf_( short line, vertex_array_holder& core,
		const Point& P0, const Point& P1, const Point& P2, const Point& P3,
		const Color& c0, const Color& c1, const Color& c2, const Color& c3,
		bool trans0, bool trans1, bool trans2, bool trans3)
{
	if( P0.is_zero()) DEBUG("pushed P0 (0,0) at %d\n",line);
	if( P1.is_zero()) DEBUG("pushed P1 (0,0) at %d\n",line);
	if( P2.is_zero()) DEBUG("pushed P2 (0,0) at %d\n",line);
	if( P3.is_zero()) DEBUG("pushed P3 (0,0) at %d\n",line);
	//interpret P0 to P3 as triangle strip
	core.push3( P0, P1, P2,
		c0, c1, c2,
		trans0, trans1, trans2);
	core.push3( P1, P2, P3,
		c1, c2, c3,
		trans1, trans2, trans3);
}

struct st_knife_cut
{
	Point T1[4]; //retained polygon, also serves as input triangle
	Color C1[4]; //
	
	Point T2[4]; //cut away polygon
	Color C2[4]; //
	
	int T1c, T2c; //count of T1 & T2
		//must be 0,3 or 4
};
int triangle_knife_cut( const Point& kn1, const Point& kn2, const Point& kn_out, //knife
			       const Color* kC0, const Color* kC1, //color of knife
		st_knife_cut& ST)//will modify for output
//see knife_cut_test for more info
{	//return number of points cut away
	int points_cut_away = 0;
	
	bool kn_colored = kC0 && kC1; //if true, use the colors of knife instead
	bool std_sign = Point::signed_area( kn1,kn2,kn_out) > 0;
	bool s1 = Point::signed_area( kn1,kn2,ST.T1[0])>0 == std_sign; //true means this point should be cut
	bool s2 = Point::signed_area( kn1,kn2,ST.T1[1])>0 == std_sign;
	bool s3 = Point::signed_area( kn1,kn2,ST.T1[2])>0 == std_sign;
	int sums = int(s1)+int(s2)+int(s3);
	
	if ( sums == 0)
	{	//all 3 points are retained
		ST.T1c = 3;
		ST.T2c = 0;
		
		points_cut_away = 0;
	}
	else if ( sums == 3)
	{	//all 3 are cut away
		ST.T1c = 0;
		ST.T2c = 3;
		
		ST.T2[0] = ST.T1[0];
		ST.T2[1] = ST.T1[1];
		ST.T2[2] = ST.T1[2];
			ST.C2[0] = ST.C1[0];
			ST.C2[1] = ST.C1[1];
			ST.C2[2] = ST.C1[2];
		
		points_cut_away = 3;
	}
	else
	{
		if ( sums == 2) {
			s1 = !s1; 
			s2 = !s2;
			s3 = !s3;
		}
		//
		Point ip1,ip2, outp;
		Color iC1,iC2, outC;
		if ( s1) { //here assume one point is cut away
				// thus only one of s1,s2,s3 is true
			outp= ST.T1[0];  outC= ST.C1[0];
			ip1 = ST.T1[1];  iC1 = ST.C1[1];
			ip2 = ST.T1[2];  iC2 = ST.C1[2];
		} else if ( s2) {
			outp= ST.T1[1];  outC= ST.C1[1];
			ip1 = ST.T1[0];  iC1 = ST.C1[0];
			ip2 = ST.T1[2];  iC2 = ST.C1[2];
		} else if ( s3) {
			outp= ST.T1[2];  outC= ST.C1[2];
			ip1 = ST.T1[0];  iC1 = ST.C1[0];
			ip2 = ST.T1[1];  iC2 = ST.C1[1];
		}
		
		Point interP1,interP2;
		Color interC1,interC2;
		double ble1,kne1, ble2,kne2;
		Point::intersect( kn1,kn2, ip1,outp, interP1, &kne1,&ble1);
		Point::intersect( kn1,kn2, ip2,outp, interP2, &kne2,&ble2);
		
		{	if ( kn_colored && Color_valid_range(kne1))
				interC1 = Color_between( *kC0, *kC1, kne1);
			else
				interC1 = Color_between( iC1, outC, ble1);
		}
		
		{	if ( kn_colored && Color_valid_range(kne2))
				interC2 = Color_between( *kC0, *kC1, kne2);
			else
				interC2 = Color_between( iC2, outC, ble2);
		}
		
		//ip2 first gives a polygon
		//ip1 first gives a triangle strip
		
		if ( sums == 1) {
			//one point is cut away
			ST.T1[0] = ip1;      ST.C1[0] = iC1;
			ST.T1[1] = ip2;      ST.C1[1] = iC2;
			ST.T1[2] = interP1;  ST.C1[2] = interC1;
			ST.T1[3] = interP2;  ST.C1[3] = interC2;
			ST.T1c = 4;
			
			ST.T2[0] = outp;     ST.C2[0] = outC;
			ST.T2[1] = interP1;  ST.C2[1] = interC1;
			ST.T2[2] = interP2;  ST.C2[2] = interC2;
			ST.T2c = 3;
			
			points_cut_away = 1;
		} else if ( sums == 2) {
			//two points are cut away
			ST.T2[0] = ip1;      ST.C2[0] = iC1;
			ST.T2[1] = ip2;      ST.C2[1] = iC2;
			ST.T2[2] = interP1;  ST.C2[2] = interC1;
			ST.T2[3] = interP2;  ST.C2[3] = interC2;
			ST.T2c = 4;
			
			ST.T1[0] = outp;     ST.C1[0] = outC;
			ST.T1[1] = interP1;  ST.C1[1] = interC1;
			ST.T1[2] = interP2;  ST.C1[2] = interC2;
			ST.T1c = 3;
			
			points_cut_away = 2;
		}
		
		/*if ( (0.0-vaser_min_alw < kne1 && kne1 < 1.0+vaser_min_alw) ||
		     (0.0-vaser_min_alw < kne2 && kne2 < 1.0+vaser_min_alw) )
		{	//highlight the wound
			glBegin(GL_LINE_STRIP);
				glColor3f(1,0,0);
				glVertex2f(ST.T1[0].x,ST.T1[0].y);
				glVertex2f(ST.T1[1].x,ST.T1[1].y);
				glVertex2f(ST.T1[2].x,ST.T1[2].y);
				glVertex2f(ST.T1[0].x,ST.T1[0].y);
			glEnd();
			
			if ( ST.T1c > 3)
			glBegin(GL_LINE_STRIP);
				glVertex2f(ST.T1[1].x,ST.T1[1].y);
				glVertex2f(ST.T1[2].x,ST.T1[2].y);
				glVertex2f(ST.T1[3].x,ST.T1[3].y);
				glVertex2f(ST.T1[1].x,ST.T1[1].y);
			glEnd();
		}*/
	}
	
	return points_cut_away;
}
void vah_knife_cut( vertex_array_holder& core, //serves as both input and output
		const Point& kn1, const Point& kn2, const Point& kn_out)
//perform knife cut on all triangles (GL_TRIANGLES) in core
{
	st_knife_cut ST;
	for ( int i=0; i<core.count; i+=3)
	{
		ST.T1[0] = core.get(i);
		ST.T1[1] = core.get(i+1);
		ST.T1[2] = core.get(i+2);
		ST.C1[0] = core.get_color(i);
		ST.C1[1] = core.get_color(i+1);
		ST.C1[2] = core.get_color(i+2);
		ST.T1c = 3; //will be ignored anyway
		
		int result = triangle_knife_cut( kn1,kn2,kn_out,0,0, ST);
		
		switch (result)
		{
		case 0:
			//do nothing
		break;
		
		case 3:	//degenerate the triangle
			core.move(i+1,i); //move i into i+1
			core.move(i+2,i);
		break;
		
		case 1:
		case 2:
			core.replace(i,  ST.T1[0],ST.C1[0]);
			core.replace(i+1,ST.T1[1],ST.C1[1]);
			core.replace(i+2,ST.T1[2],ST.C1[2]);
			
			if ( result==1)
			{	//create a new triangle
				Point dump_P;
				Color dump_C;
				int a1,a2,a3;
				a1 = core.push( dump_P, dump_C);
				a2 = core.push( dump_P, dump_C);
				a3 = core.push( dump_P, dump_C);
				
				//copy the original points
				core.move( a1, i+1);
				core.move( a2, i+2);
				core.move( a3, i+2);
				
				//make the new point
				core.replace( a3, ST.T1[3],ST.C1[3]);
			}
		break;
		
		}
	}
}
void vah_N_knife_cut( vertex_array_holder& in, vertex_array_holder& out,
		const Point* kn0, const Point* kn1, const Point* kn2,
		const Color* kC0, const Color* kC1,
		int N)
{	//an iterative implementation
	const int MAX_ST = 10;
	st_knife_cut ST[MAX_ST];
	
	bool kn_colored = kC0 && kC1;
	
	if ( N > MAX_ST)
	{
		DEBUG("vah_N_knife_cut: max N for current build is %d\n", MAX_ST);
		N = MAX_ST;
	}
	
	for ( int i=0; i<in.count; i+=3) //each input triangle
	{
		int ST_count = 1;
		ST[0].T1[0] = in.get(i);
		ST[0].T1[1] = in.get(i+1);
		ST[0].T1[2] = in.get(i+2);
			ST[0].C1[0] = in.get_color(i);
			ST[0].C1[1] = in.get_color(i+1);
			ST[0].C1[2] = in.get_color(i+2);
		ST[0].T1c = 3;
		
		for ( int k=0; k<N; k++) //each knife
		{
			int cur_count = ST_count;
			for ( int p=0; p<cur_count; p++) //each triangle to be cut
			{
				//perform cut
				if ( ST[p].T1c > 0)
					if ( kn_colored)
					triangle_knife_cut( kn0[k], kn1[k], kn2[k],
							   &kC0[k],&kC1[k],
								ST[p]);
					else
					triangle_knife_cut( kn0[k],kn1[k],kn2[k],
							    0,0,ST[p]);
				
				//push retaining part
				if ( ST[p].T1c > 0) {
					out.push( ST[p].T1[0], ST[p].C1[0]);
					out.push( ST[p].T1[1], ST[p].C1[1]);
					out.push( ST[p].T1[2], ST[p].C1[2]);
					if ( ST[p].T1c > 3) {
						out.push( ST[p].T1[1], ST[p].C1[1]);
						out.push( ST[p].T1[2], ST[p].C1[2]);
						out.push( ST[p].T1[3], ST[p].C1[3]);
					}
				}
				
				//store cut away part to be cut again
				if ( ST[p].T2c > 0)
				{
					ST[p].T1[0] = ST[p].T2[0];
					ST[p].T1[1] = ST[p].T2[1];
					ST[p].T1[2] = ST[p].T2[2];
						ST[p].C1[0] = ST[p].C2[0];
						ST[p].C1[1] = ST[p].C2[1];
						ST[p].C1[2] = ST[p].C2[2];
					ST[p].T1c = 3;
					
					if ( ST[p].T2c > 3)
					{
						ST[ST_count].T1[0] = ST[p].T2[1];
						ST[ST_count].T1[1] = ST[p].T2[2];
						ST[ST_count].T1[2] = ST[p].T2[3];
							ST[ST_count].C1[0] = ST[p].C2[1];
							ST[ST_count].C1[1] = ST[p].C2[2];
							ST[ST_count].C1[2] = ST[p].C2[3];
						ST[ST_count].T1c = 3;
						ST_count++;
					}
				}
				else
				{
					ST[p].T1c = 0;
				}
			}
		}
	}
}

const float cri_core_adapt = 0.0001f;
void anchor_late( const Vec2* P, const Color* C, st_polyline* SL,
		vertex_array_holder& tris,
		Point cap1, Point cap2)
{	const int size_of_P = 3;

	tris.set_gl_draw_mode(GL_TRIANGLES);
	
	Point P_0, P_1, P_2;
	P_0 = Point(P[0]);
	P_1 = Point(P[1]);
	P_2 = Point(P[2]);
	if ( SL[0].djoint==PLC_butt || SL[0].djoint==PLC_square)
		P_0 -= cap1;
	if ( SL[2].djoint==PLC_butt || SL[2].djoint==PLC_square)
		P_2 -= cap2;
	
	Point P0, P1, P2, P3, P4, P5, P6, P7;
	Point P0r,P1r,P2r,P3r,P4r,P5r,P6r,P7r; //fade
	
	P0 = P_1 + SL[1].vP;
		P0r = P0 + SL[1].vR;
	P1 = P_1 - SL[1].vP;
		P1r = P1 - SL[1].vR;
	
	P2 = P_1 + SL[1].T1;
		P2r = P2 + SL[1].R1 + SL[0].bR;
	P3 = P_0 + SL[0].T;
		P3r = P3 + SL[0].R;
	P4 = P_0 - SL[0].T;
		P4r = P4 - SL[0].R;
	
	P5 = P_1 + SL[1].T;
		P5r = P5 + SL[1].R - SL[1].bR;
	P6 = P_2 + SL[2].T;
		P6r = P6 + SL[2].R;
	P7 = P_2 - SL[2].T;
		P7r = P7 - SL[2].R;
		/* annotate( P0,C[0],0);
		annotate( P1);
		annotate( P2);
		annotate( P3);
		annotate( P4);
		annotate( P5);
		annotate( P6);
		annotate( P7); */

	int normal_line_core_joint = 1; //0:dont draw, 1:draw, 2:outer only

	//consider these as inline child functions
	#define normal_first_segment \
			tris.push3( P3,  P2,  P1, \
				  C[0],C[1],C[1]);\
			tris.push3( P1,  P3,  P4, \
				  C[1],C[0],C[0])
	
	#define normal_last_segment \
			tris.push3( P1,  P5,  P6, \
				  C[1],C[1],C[2]);\
			tris.push3( P1,  P6,  P7, \
				  C[1],C[2],C[2])
	
	Color Cpt; //color at PT
	if ( SL[1].degenT || SL[1].degenR)
	{
		float pt = sqrt(SL[1].pt);
		if ( SL[1].pre_full)
			Cpt = Color_between(C[0],C[1], pt);
		else
			Cpt = Color_between(C[1],C[2], 1-pt);
	}
	
	if ( SL[1].degenT)
	{	//degen line core
		P1 = SL[1].PT;
			if( SL[1].degenR)
				P1r = SL[1].PR;
		
		tris.push3( P3,  P2,  P1,
			  C[0],C[1],C[1]); //fir seg
		tris.push3( P1,  P5,  P6,
			  C[1],C[1],C[2]); //las seg
		
		if ( SL[1].pre_full)
		{	tris.push3( P1,  P3,  P4,
				  C[1],C[0],C[0]);
		}
		else
		{	tris.push3( P1,  P6,  P7,
				  C[1],C[2],C[2]);
		}
	}
	else if ( SL[1].degenR && SL[1].pt > cri_core_adapt) //&& ! SL[1].degenT
	{	//line core adapted for degenR
		if ( SL[1].pre_full)
		{
			normal_last_segment;

			//special first segment
			Point P9 = SL[1].PT;
			tris.push3( P3,  P2,  P1,
				  C[0],C[1],C[1]);
			tris.push3( P3,  P9,  P1,
				  C[0], Cpt,C[1]);
			tris.push3( P3,  P9,  P4,
				  C[0], Cpt,C[0]);
		}
		else
		{
			normal_first_segment;
			
			//special last segment
			Point P9 = SL[1].PT;
			push_quad( tris,
				  P5,  P1,  P6,  P9,
				C[1],C[1],C[2], Cpt);
			tris.push3( P7,  P9,  P6,
				  C[2], Cpt,C[2]);
				/*annotate(P1,C[1],1);
				annotate(P5,C[1],5);
				annotate(P6,C[1],6);
				annotate(P7,C[1],7);
				annotate(P9,C[1],9);*/
		}
	}
	else
	{
		normal_first_segment;
		normal_last_segment;
	#undef normal_first_segment
	#undef normal_last_segment
	}
	
	if (normal_line_core_joint)
	{
		switch( SL[1].djoint)
		{
			case PLJ_miter:
				tris.push3( P2,  P5,  P0,
					  C[1],C[1],C[1]);
			case PLJ_bevel:
				if ( normal_line_core_joint==1)
				tris.push3( P2,  P5,  P1,
					  C[1],C[1],C[1]);
			break;
			
			case PLJ_round: {
				vertex_array_holder strip;
				strip.set_gl_draw_mode(GL_TRIANGLE_STRIP);
				
			if ( normal_line_core_joint==1)
				vectors_to_arc( strip, P_1, C[1], C[1],
				SL[1].T1, SL[1].T,
				get_PLJ_round_dangle(SL[1].t,SL[1].r),
				SL[1].t, 0.0f, false, &P1);
			else if ( normal_line_core_joint==2)
				vectors_to_arc( strip, P_1, C[1], C[1],
				SL[1].T1, SL[1].T,
				get_PLJ_round_dangle(SL[1].t,SL[1].r),
				SL[1].t, 0.0f, false, &P5);
				
				tris.push(strip);
			} break;
		}
	}
	
	if ( SL[1].degenR)
	{	//degen inner fade
		Point P9 = SL[1].PT;
			Point P9r = SL[1].PR;
		//annotate(P9,C[0],9);
		//annotate(P9r);

		Color ccpt=Cpt;
		if ( SL[1].degenT)
			ccpt = C[1];
		
		if ( SL[1].pre_full)
		{	push_quadf( tris,
				    P9,  P4, P9r, P4r,
				  ccpt,C[0],C[1],C[0],
				     0,   0,   1,   1); //fir seg

			if ( !SL[1].degenT)
			{
				Point mid = Point::midpoint(P9,P7);
				tris.push3( P1,  P9, mid,
					  C[1], Cpt,C[1],
					     0,   0,   1);
				tris.push3( P1,  P7, mid,
					  C[1],C[2],C[1],
					     0,   0,   1);
			}
		}
		else
		{	push_quadf( tris,
				    P9,  P7, P9r, P7r,
				  ccpt,C[2],C[1],C[2],
				     0,   0,   1,   1); //las seg
			
			if ( !SL[1].degenT)
			{
				Point mid = Point::midpoint(P9,P4);
				tris.push3( P1,  P9, mid,
					  C[1], Cpt,C[1],
					     0,   0,   1);
				tris.push3( P1,  P4, mid,
					  C[1],C[0],C[1],
					     0,   0,   1);
			}
		}
	}
	else
	{	//normal inner fade
		push_quadf( tris,
			    P1,  P4, P1r, P4r,
			  C[1],C[0],C[1],C[0],
			     0,   0,   1,   1); //fir seg
		push_quadf( tris,
			    P1,  P7, P1r, P7r,
			  C[1],C[2],C[1],C[2],
			     0,   0,   1,   1); //las seg
	}
	
	{	//outer fade, whether degen or normal
		push_quadf( tris,
			    P2,  P3, P2r, P3r,
			  C[1],C[0],C[1],C[0],
			     0,   0,   1,   1); //fir seg
		push_quadf( tris,
			    P5,  P6, P5r, P6r,
			  C[1],C[2],C[1],C[2],
			     0,   0,   1,   1); //las seg
		switch( SL[1].djoint)
		{	//line fade joint
			case PLJ_miter:
				push_quadf( tris,
					    P0,  P5, P0r, P5r,
					  C[1],C[1],C[1],C[1],
					     0,   0,   1,   1);
				push_quadf( tris,
					    P0,  P2, P0r, P2r,
					  C[1],C[1],C[1],C[1],
					     0,   0,   1,   1);
			break;
			case PLJ_bevel:
				push_quadf( tris,
					    P2,  P5, P2r, P5r,
					  C[1],C[1],C[1],C[1],
					     0,   0,   1,   1);
			break;
			
			case PLJ_round: {
				vertex_array_holder strip;
				strip.set_gl_draw_mode(GL_TRIANGLE_STRIP);
				Color C2 = C[1]; C2.a = 0.0f;
				vectors_to_arc( strip, P_1, C[1], C2,
				SL[1].T1, SL[1].T,
				get_PLJ_round_dangle(SL[1].t,SL[1].r),
				SL[1].t, SL[1].t+SL[1].r, false, 0);
				
				tris.push(strip);
			} break;
		}
	}
} //anchor_late

void anchor_cap( const Vec2* P, const Color* C, st_polyline* SL,
		vertex_array_holder& tris,
		Point cap1, Point cap2)
{
	Point P4 = Point(P[0]) - SL[0].T;
	Point P7 = Point(P[2]) - SL[2].T;
	for ( int i=0,k=0; k<=1; i=2, k++)
	{
		vertex_array_holder cap;
		Point& cur_cap = i==0? cap1:cap2;
		if ( cur_cap.non_zero())
		{
			cap.set_gl_draw_mode(GL_TRIANGLES);
			bool perform_cut = ( SL[1].degenR && SL[1].R_full_degen) &&
					((k==0 && !SL[1].pre_full) ||
					 (k==1 &&  SL[1].pre_full) );
			
			Point P3 = Point(P[i])-SL[i].T*2-SL[i].R+cur_cap;
			
			if ( SL[i].djoint == PLC_round)
			{	//round caps
				{	vertex_array_holder strip;
					strip.set_gl_draw_mode(GL_TRIANGLE_STRIP);
					
					Color C2 = C[i]; C2.a = 0.0f;
					Point O  = Point(P[i]);
					Point app_P = O+SL[i].T;
					Point bR = SL[i].bR;
					bR.follow_signs(cur_cap);
					float dangle = get_PLJ_round_dangle(SL[i].t,SL[i].r);
					
					vectors_to_arc( strip, O, C[i], C[i],
					SL[i].T+bR, -SL[i].T+bR,
					dangle,
					SL[i].t, 0.0f, false, &app_P);
						strip.push( O-SL[i].T,C[i]);
						strip.push( app_P,C[i]);

					strip.jump();

					Point a1 = O+SL[i].T;
					Point a2 = O+SL[i].T*(1/SL[i].t)*(SL[i].t+SL[i].r);
					Point b1 = O-SL[i].T;
					Point b2 = O-SL[i].T*(1/SL[i].t)*(SL[i].t+SL[i].r);
					
						strip.push( a1,C[i]);
						strip.push( a2,C2);
					vectors_to_arc( strip, O, C[i], C2,
					SL[i].T+bR, -SL[i].T+bR,
					dangle,
					SL[i].t, SL[i].t+SL[i].r, false, 0);						
						strip.push( b1,C[i]);
						strip.push( b2,C2);
					cap.push(strip);
				}
				if ( perform_cut)
				{
					Point P4k;
					if ( !SL[1].pre_full)
						P4k = P7; //or P7r ?
					else
						P4k = P4;
					
					vah_knife_cut( cap, SL[1].PT, P4k, P3);
					/*annotate(SL[1].PT,C[i],0);
					annotate(P3,C[i],3);
					annotate(P4k,C[i],4);*/
				}
			}
			else //if ( SL[i].djoint == PLC_butt | SL[i].cap == PLC_square | SL[i].cap == PLC_rect)
			{	//rectangle caps
				Point P_cur = P[i];
				bool degen_nxt=0, degen_las=0;
				if ( k == 0)
					if ( SL[0].djoint==PLC_butt || SL[0].djoint==PLC_square)
						P_cur -= cap1;
				if ( k == 1)
					if ( SL[2].djoint==PLC_butt || SL[2].djoint==PLC_square)
						P_cur -= cap2;
				
				Point P0,P1,P2,P3,P4,P5,P6;
				
				P0 = P_cur+SL[i].T+SL[i].R;
				P1 = P0+cur_cap;
				P2 = P_cur+SL[i].T;
				P4 = P_cur-SL[i].T;
				P3 = P4-SL[i].R+cur_cap;
				P5 = P4-SL[i].R;
				
				cap.push( P0, C[i],true);
				cap.push( P1, C[i],true);
				cap.push( P2, C[i]);
				
						cap.push( P1, C[i],true);
					cap.push( P2, C[i]);
				cap.push( P3, C[i],true);
				
						cap.push( P2, C[i]);
					cap.push( P3, C[i],true);
				cap.push( P4, C[i]);
				
						cap.push( P3, C[i],true);
					cap.push( P4, C[i]);
				cap.push( P5, C[i],true);
				//say if you want to use triangle strip,
				//  just push P0~ P5 in sequence
				if ( perform_cut)
				{
					vah_knife_cut( cap, SL[1].PT, SL[1].PR, P3);
					/*annotate(SL[1].PT,C[i],0);
					annotate(SL[1].PR);
					annotate(P3);
					annotate(P4);*/
				}
			}
		}
		tris.push(cap);
	}
} //anchor_cap

void segment_late( const Vec2* P, const Color* C, st_polyline* SL,
		vertex_array_holder& tris,
		Point cap1, Point cap2)
{
	tris.set_gl_draw_mode(GL_TRIANGLES);
	
	Point P_0, P_1, P_2;
	P_0 = Point(P[0]);
	P_1 = Point(P[1]);
	if ( SL[0].djoint==PLC_butt || SL[0].djoint==PLC_square)
		P_0 -= cap1;
	if ( SL[1].djoint==PLC_butt || SL[1].djoint==PLC_square)
		P_1 -= cap2;
	
	Point P1, P2, P3, P4;  //core
	Point P1c,P2c,P3c,P4c; //cap
	Point P1r,P2r,P3r,P4r; //fade

	P1 = P_0 + SL[0].T;
		P1r = P1 + SL[0].R;
		P1c = P1r + cap1;
	P2 = P_0 - SL[0].T;
		P2r = P2 - SL[0].R;
		P2c = P2r + cap1;
	P3 = P_1 + SL[1].T;
		P3r = P3 + SL[1].R;
		P3c = P3r + cap2;
	P4 = P_1 - SL[1].T;
		P4r = P4 - SL[1].R;
		P4c = P4r + cap2;
	//core
	push_quad( tris,
		  P1,  P2,  P3,  P4,
		C[0],C[0],C[1],C[1] );
	//fade
	push_quadf( tris,
		  P1, P1r,  P3, P3r,
		C[0],C[0],C[1],C[1],
		   0,   1,   0,   1 );
	push_quadf( tris,
		  P2, P2r,  P4, P4r,
		C[0],C[0],C[1],C[1],
		   0,   1,   0,   1 );
	//caps
	for ( int j=0; j<2; j++)
	{
		vertex_array_holder cap;
		cap.set_gl_draw_mode(GL_TRIANGLE_STRIP);
		Point &cur_cap = j==0?cap1:cap2;
		if( cur_cap.is_zero())
			continue;
		
		if ( SL[j].djoint == PLC_round)
		{	//round cap
			Color C2 = C[j]; C2.a = 0.0f;
			Point O  = Point(P[j]);
			Point app_P = O+SL[j].T;
			Point bR = SL[j].bR;
			bR.follow_signs( j==0?cap1:cap2);
			float dangle = get_PLJ_round_dangle(SL[j].t,SL[j].r);
			
			vectors_to_arc( cap, O, C[j], C[j],
			SL[j].T+bR, -SL[j].T+bR,
			dangle,
			SL[j].t, 0.0f, false, &app_P);
				cap.push( O-SL[j].T,C[j]);
				cap.push( app_P,C[j]);
			
			cap.jump();
			
			{	//fade
				Point a1 = O+SL[j].T;
				Point a2 = O+SL[j].T*(1/SL[j].t)*(SL[j].t+SL[j].r);
				Point b1 = O-SL[j].T;
				Point b2 = O-SL[j].T*(1/SL[j].t)*(SL[j].t+SL[j].r);
				
					cap.push( a1,C[j]);
					cap.push( a2,C2);
				vectors_to_arc( cap, O, C[j], C2,
				SL[j].T+bR, -SL[j].T+bR,
				dangle,
				SL[j].t, SL[j].t+SL[j].r, false, 0);						
					cap.push( b1,C[j]);
					cap.push( b2,C2);
			}
		}
		else //if ( SL[j].djoint == PLC_butt | SL[j].cap == PLC_square | SL[j].cap == PLC_rect)
		{	//rectangle cap
			Point Pj,Pjr,Pjc, Pk,Pkr,Pkc;
			if ( j==0)
			{
				Pj = P1;
				Pjr= P1r;
				Pjc= P1c;
				
				Pk = P2;
				Pkr= P2r;
				Pkc= P2c;
			}
			else
			{
				Pj = P3;
				Pjr= P3r;
				Pjc= P3c;
				
				Pk = P4;
				Pkr= P4r;
				Pkc= P4c;
			}
			
			cap.push( Pkr, C[j], 1);
			cap.push( Pkc, C[j], 1);
			cap.push( Pk , C[j], 0);
			cap.push( Pjc, C[j], 1);
			cap.push( Pj , C[j], 0);
			cap.push( Pjr, C[j], 1);
		}
		tris.push(cap);
	}
	
	/*annotate(P1,C[0],1);
	annotate(P2,C[0],2);
	annotate(P3,C[0],3);
	annotate(P4,C[0],4);
		annotate(P1c,C[0],11);
		annotate(P2c,C[0],21);
		annotate(P3c,C[0],31);
		annotate(P4c,C[0],41);
		
		annotate(P1r,C[0],12);
		annotate(P2r,C[0],22);
		annotate(P3r,C[0],32);
		annotate(P4r,C[0],42);
	*/
}

void segment( st_anchor& SA, const polyline_opt* options,  bool cap_first, bool cap_last, char last_cap_type=-1)
{
	double* weight = SA.W;
	if ( !SA.P || !SA.C || !weight) return;
	
	Point P[2]; P[0]=SA.P[0]; P[1]=SA.P[1];
	Color C[2]; C[0]=SA.C[0]; C[1]=SA.C[1];
	
	polyline_opt opt={0};
	if ( options)
		opt = (*options);
	
	Point T1,T2;
	Point R1,R2;
	Point bR;
	double t,r;
	
	bool varying_weight = !(weight[0]==weight[1]);
	
	Point cap_start, cap_end;
	st_polyline SL[2];
	
	for ( int i=0; i<2; i++)
	{
		if ( weight[i]>=0.0 && weight[i]<1.0)
		{
			double f=weight[i]-static_cast<int>(weight[i]);
			C[i].a *=f;
		}
	}
	
	{	int i=0;
		make_T_R_C( P[i], P[i+1], &T2,&R2,&bR, weight[i],opt, &r,&t,0, true);
		
		if ( cap_first)
		{
			if ( opt.cap==PLC_square)
			{
				P[0] = Point(P[0]) - bR * (t+r);
			}
			cap_start = bR;
			cap_start.opposite(); if ( opt.feather && !opt.no_feather_at_cap)
			cap_start*=opt.feathering;
		}
		
		SL[i].djoint=opt.cap;
		SL[i].t=t;
		SL[i].r=r;
		SL[i].T=T2;
		SL[i].R=R2;
		SL[i].bR=bR*0.01;
		SL[i].degenT = false;
		SL[i].degenR = false;
	}
	
	{	int i=1;
		if ( varying_weight)
			make_T_R_C( P[i-1], P[i], &T2,&R2,&bR,weight[i],opt, &r,&t,0, true);
		
		last_cap_type = last_cap_type==-1 ? opt.cap:last_cap_type;
		
		if ( cap_last)
		{
			if ( last_cap_type==PLC_square)
			{
				P[1] = Point(P[1]) + bR * (t+r);
			}
			cap_end = bR;
			if ( opt.feather && !opt.no_feather_at_cap)
				cap_end*=opt.feathering;
		}
		
		SL[i].djoint = last_cap_type;
		SL[i].t=t;
		SL[i].r=r;
		SL[i].T=T2;
		SL[i].R=R2;
		SL[i].bR=bR*0.01;
		SL[i].degenT = false;
		SL[i].degenR = false;
	}
	
	segment_late( P,C,SL,SA.vah, cap_start,cap_end);
}

int anchor( st_anchor& SA, const polyline_opt* options, bool cap_first, bool cap_last)
{
	polyline_opt opt={0};
	if ( options)
		opt = (*options);
	
	Point* P = SA.P;
	Color* C = SA.C;
	double* weight = SA.W;

	{	st_polyline emptySL;
		SA.SL[0]=emptySL; SA.SL[1]=emptySL; SA.SL[2]=emptySL;
	}
	st_polyline* SL = SA.SL;
	SA.vah.set_gl_draw_mode(GL_TRIANGLES);
	SA.cap_start = Point();
	SA.cap_end = Point();
	
	//const double critical_angle=11.6538;
	//	critical angle in degrees where a miter is force into bevel
	//	it is _similar_ to cairo_set_miter_limit () but cairo works with ratio while VASEr works with included angle
	const double cos_cri_angle=0.979386; //cos(critical_angle)
	
	bool varying_weight = !(weight[0]==weight[1] & weight[1]==weight[2]);
	
	double combined_weight=weight[1]+(opt.feather?opt.feathering:0.0);
	if ( combined_weight < cri_segment_approx)
	{
		segment( SA, &opt, cap_first,false, opt.joint==PLJ_round?PLC_round:PLC_butt);
		char ori_cap = opt.cap;
		opt.cap = opt.joint==PLJ_round?PLC_round:PLC_butt;
		SA.P[0]=SA.P[1]; SA.P[1]=SA.P[2];
		SA.C[0]=SA.C[1]; SA.C[1]=SA.C[2];
		SA.W[0]=SA.W[1]; SA.W[1]=SA.W[2];
		segment( SA, &opt, false,cap_last, ori_cap);
		return 0;
	}
	
	Point T1,T2,T21,T31;		//]these are for calculations in early stage
	Point R1,R2,R21,R31;		//]
	
	for ( int i=0; i<3; i++)
	{	//lower the transparency for weight < 1.0
		if ( weight[i]>=0.0 && weight[i]<1.0)
		{
			double f=weight[i];
			C[i].a *=f;
		}
	}
	
	{	int i=0;
	
		Point cap1;
		double r,t;		
		make_T_R_C( P[i], P[i+1], &T2,&R2,&cap1, weight[i],opt, &r,&t,0);
		if ( varying_weight) {
		make_T_R_C( P[i], P[i+1], &T31,&R31,0, weight[i+1],opt, 0,0,0);
		} else {
			T31 = T2;
			R31 = R2;
		}
		Point::anchor_outward(R2, P[i+1],P[i+2] /*,inward_first->value()*/);
			T2.follow_signs(R2);
		
		SL[i].bR=cap1;
		
		if ( cap_first)
		{
			if ( opt.cap==PLC_square)
			{
				P[0] = Point(P[0]) - cap1 * (t+r);
			}
			cap1.opposite(); if ( opt.feather && !opt.no_feather_at_cap)
			cap1*=opt.feathering;
			SA.cap_start = cap1;
		}
		
		SL[i].djoint=opt.cap;
		SL[i].T=T2;
		SL[i].R=R2;
		SL[i].t=(float)t;
		SL[i].r=(float)r;
		SL[i].degenT = false;
		SL[i].degenR = false;
		
		SL[i+1].T1=T31;
		SL[i+1].R1=R31;
	}
	
	if ( cap_last)
	{	int i=2;

		Point cap2;
		double t,r;
		make_T_R_C( P[i-1],P[i], 0,0,&cap2,weight[i],opt, &r,&t,0);
		if ( opt.cap==PLC_square)
		{
			P[2] = Point(P[2]) + cap2 * (t+r);
		}
		
		SL[i].bR=cap2;
		
		if ( opt.feather && !opt.no_feather_at_cap)
			cap2*=opt.feathering;
		SA.cap_end = cap2;
	}
	
	{	int i=1;
	
		double r,t;
		Point P_cur = P[i]; //current point //to avoid calling constructor repeatedly
		Point P_nxt = P[i+1]; //next point
		Point P_las = P[i-1]; //last point
		if ( opt.cap==PLC_butt || opt.cap==PLC_square)
		{
			P_nxt -= SA.cap_end;
			P_las -= SA.cap_start;
		}
		
		{
		Point bR; float length_cur, length_nxt;
		make_T_R_C( P_las, P_cur,  &T1,&R1, 0, weight[i-1],opt,0,0, &length_cur);
		if ( varying_weight) {
		make_T_R_C( P_las, P_cur, &T21,&R21,0, weight[i],opt,  0,0,0);
		} else {
			T21 = T1;
			R21 = R1;
		}
		
		make_T_R_C( P_cur, P_nxt,  &T2,&R2,&bR, weight[i],opt, &r,&t, &length_nxt);
		if ( varying_weight) {
		make_T_R_C( P_cur, P_nxt, &T31,&R31,0, weight[i+1],opt, 0,0,0);
		} else {
			T31 = T2;
			R31 = R2;
		}
		
		SL[i].T=T2;
		SL[i].R=R2;
		SL[i].bR=bR;
		SL[i].t=(float)t;
		SL[i].r=(float)r;
		SL[i].degenT = false;
		SL[i].degenR = false;
		
		SL[i+1].T1=T31;
		SL[i+1].R1=R31;
		}
		
		{	//2nd to 2nd last point
			
			//find the angle between the 2 line segments
			Point ln1,ln2, V;
			ln1 = P_cur - P_las;
			ln2 = P_nxt - P_cur;
			ln1.normalize();
			ln2.normalize();
			Point::dot(ln1,ln2, V);
			double cos_tho=-V.x-V.y;
			bool zero_degree = Point::negligible(cos_tho-1);
			bool d180_degree = cos_tho < -1+0.0001;
			bool smaller_than_30_degree = cos_tho > 0.8660254;
			char result3 = 1;
			
			if ( (cos_tho < 0 && opt.joint==PLJ_bevel) ||
			     (opt.joint!=PLJ_bevel && opt.cap==PLC_round) ||
			     (opt.joint==PLJ_round)
			   )
			{	//when greater than 90 degrees
				SL[i-1].bR *= 0.01;
				SL[i]  .bR *= 0.01;
				SL[i+1].bR *= 0.01;
				//to solve an overdraw in bevel and round joint
			}
			
			Point::anchor_outward( T1, P_cur,P_nxt);
				R1.follow_signs(T1);
			Point::anchor_outward( T21, P_cur,P_nxt);
				R21.follow_signs(T21);
				SL[i].T1.follow_signs(T21);
				SL[i].R1.follow_signs(T21);
			Point::anchor_outward( T2, P_cur,P_las);
				R2.follow_signs(T2);
				SL[i].T.follow_signs(T2);
				SL[i].R.follow_signs(T2);
			Point::anchor_outward( T31, P_cur,P_las);
				R31.follow_signs(T31);
			
			{ //must do intersection
				Point interP, vP;
				result3 = Point::intersect( P_las+T1, P_cur+T21,
							P_nxt+T31, P_cur+T2,
							interP);
				
				if ( result3) {
					vP = interP - P_cur;
					SL[i].vP=vP;
					SL[i].vR=vP*(r/t);
				} else {
					SL[i].vP=SL[i].T;
					SL[i].vR=SL[i].R;
					DEBUG( "intersection failed: cos(angle)=%.4f, angle=%.4f(degree)\n", cos_tho, acos(cos_tho)*180/3.14159);
				}
			}
			
			T1.opposite();		//]inward
				R1.opposite();
			T21.opposite();
				R21.opposite();
			T2.opposite();
				R2.opposite();
			T31.opposite();
				R31.opposite();
			
			//make intersections
			Point PR1,PR2, PT1,PT2;
			double pt1,pt2;
			
			char result1r = Point::intersect( P_nxt-T31-R31, P_nxt+T31+R31,
						P_las+T1+R1, P_cur+T21+R21, //knife1
						PR1); //fade
			char result2r = Point::intersect( P_las-T1-R1, P_las+T1+R1,
						P_nxt+T31+R31, P_cur+T2+R2, //knife2
						PR2);
			bool is_result1r = result1r == 1;
			bool is_result2r = result2r == 1;
			//
			char result1t = Point::intersect( P_nxt-T31, P_nxt+T31,
						P_las+T1, P_cur+T21, //knife1_a
						PT1, 0,&pt1); //core
			char result2t = Point::intersect( P_las-T1, P_las+T1,
						P_nxt+T31, P_cur+T2, //knife2_a
						PT2, 0,&pt2);
			bool is_result1t = result1t == 1;
			bool is_result2t = result2t == 1;
			//
			bool inner_sec = Point::intersecting( P_las+T1+R1, P_cur+T21+R21,
						P_nxt+T31+R31, P_cur+T2+R2);
			//
			if ( zero_degree)
			{
				bool pre_full = is_result1t;
				opt.no_feather_at_cap=true;
				if ( pre_full)
				{
					segment( SA, &opt, true,cap_last, opt.joint==PLJ_round?PLC_round:PLC_butt);
				}
				else
				{
					char ori_cap = opt.cap;
					opt.cap = opt.joint==PLJ_round?PLC_round:PLC_butt;
					SA.P[0]=SA.P[1]; SA.P[1]=SA.P[2];
					SA.C[0]=SA.C[1]; SA.C[1]=SA.C[2];
					SA.W[0]=SA.W[1]; SA.W[1]=SA.W[2];
					segment( SA, &opt, true,cap_last, ori_cap);
				}
				return 0;
			}
			
			if ( (is_result1r | is_result2r) && !inner_sec)
			{	//fade degeneration
				SL[i].degenR=true;
				SL[i].PT = is_result1r? PT1:PT2; //this is is_result1r!!
				SL[i].PR = is_result1r? PR1:PR2;
				SL[i].pt = float(is_result1r? pt1:pt2);
					if ( SL[i].pt < 0)
						SL[i].pt = cri_core_adapt;
				SL[i].pre_full = is_result1r;
				SL[i].R_full_degen = false;
				
				Point P_nxt = P[i+1]; //override that in the parent scope
				Point P_las = P[i-1];
				Point PR;
				if ( opt.cap==PLC_rect || opt.cap==PLC_round)
				{
					P_nxt += SA.cap_end;
					P_las += SA.cap_start;
				}
				char result2;
				if ( is_result1r)
				{
					result2 = Point::intersect( P_nxt-T31-R31, P_nxt+T31,
						P_las+T1+R1, P_cur+T21+R21, //knife1
						PR); 	//fade
				}
				else
				{
					result2 = Point::intersect( P_las-T1-R1, P_las+T1,
						P_nxt+T31+R31, P_cur+T2+R2, //knife2
						PR);
				}
				if ( result2 == 1)
				{
					SL[i].R_full_degen = true;
					SL[i].PR = PR;
				}
			}
			
			if ( is_result1t | is_result2t)
			{	//core degeneration
				SL[i].degenT=true;
				SL[i].pre_full=is_result1t;
				SL[i].PT = is_result1t? PT1:PT2;
				SL[i].pt = float(is_result1t? pt1:pt2);
			}
			
			//make joint
			SL[i].djoint = opt.joint;
			if ( opt.joint == PLJ_miter)
				if ( cos_tho >= cos_cri_angle)
					SL[i].djoint=PLJ_bevel;
			
			/*if ( varying_weight && smaller_than_30_degree)
			{	//not sure why, but it appears to solve a visual bug for varing weight
				Point interR,vR;
				char result3 = Point::intersect( P_las-T1-R1, P_cur-T21-R21,
							P_nxt-T31-R31, P_cur-T2-R2,
							interR);
				SL[i].vR = P_cur-interR-SL[i].vP;
				annotate(interR,C[i],9);
				draw_vector(P_las-T1-R1, P_cur-T21-R21 - P_las+T1+R1,"1");
				draw_vector(P_nxt-T31-R31, P_cur-T2-R2 - P_nxt+T31+R31,"2");
			}*/
			
			if ( d180_degree | !result3)
			{	//to solve visual bugs 3 and 1.1
				//efficiency: if color and weight is same as previous and next point
				// ,do not generate vertices
				same_side_of_line( SL[i].R, SL[i-1].R, P_cur,P_las);
					SL[i].T.follow_signs(SL[i].R);
				SL[i].vP=SL[i].T;
				SL[i].T1.follow_signs(SL[i].T);
				SL[i].R1.follow_signs(SL[i].T);
				SL[i].vR=SL[i].R;
				SL[i].djoint=PLJ_miter;
			}
		} //2nd to 2nd last point
	}
	
	{	int i=2;

		double r,t;
		make_T_R_C( P[i-1],P[i], &T2,&R2,0,weight[i],opt,  &r,&t,0);
			same_side_of_line( R2, SL[i-1].R, P[i-1],P[i]);
				T2.follow_signs(R2);
		
		SL[i].djoint=opt.cap;
		SL[i].T=T2;
		SL[i].R=R2;
		SL[i].t=(float)t;
		SL[i].r=(float)r;
		SL[i].degenT = false;
		SL[i].degenR = false;
	}
	
	if( cap_first || cap_last)
		anchor_cap( SA.P,SA.C, SA.SL,SA.vah, SA.cap_start,SA.cap_end);
	anchor_late( SA.P,SA.C, SA.SL,SA.vah, SA.cap_start,SA.cap_end);
	return 1;
} //anchor

#ifdef VASE_RENDERER_EXPER
template <typename T>
class circular_array
{
	const int size;
	int cur; //current
	T* array;
public:
	circular_array(int size_) : size(size_)
	{
		array = new T[size];
		cur = 0;
	}
	
	~circular_array()
	{
		delete[] array;
	}
	
	void push( T obj)
	{
		array[cur] = obj;
		move(1);
	}
	
	int get_size() const
		{ return size;}
	
	int get_i( int i) const //get valid index relative to current
	{
		int des = cur + i%size;
		if ( des > size-1)
		{
			des -= size;
		}
		if ( des < 0)
		{
			des = size+i;
		}
		return des;
	}
	
	void move( int i) //move current relatively
	{
		cur = get_i(i);
	}
	
	T& operator[] (int i) //get element at relative position
	{
		return array[get_i(i)];
	}
};
#endif //VASE_RENDERER_EXPER

struct polyline_inopt
{
	bool const_color;
	bool const_weight;
	bool no_cap_first;
	bool no_cap_last;
	bool join_first;
	bool join_last;
	double* segment_length; //array of length of each segment
};

void poly_point_inter( const Point* P, const Color* C, const double* W, const polyline_inopt* inopt,
		Point& p, Color& c, double& w,
		int at, double t)
{
	#define color(I)  C[inopt&&inopt->const_color?0: I]
	#define weight(I) W[inopt&&inopt->const_weight?0: I]
	if( t==0.0)
	{
		p = P[at];
		c = color(at);
		w = weight(at);
	}
	else if( t==1.0)
	{
		p = P[at+1];
		c = color(at+1);
		w = weight(at+1);
	}
	else
	{
		p = (P[at]+P[at+1]) * t;
		c = Color_between(color(at),color(at+1), t);
		w = (weight(at)+weight(at+1)) * t;
	}
	#undef color
	#undef weight
}

void polyline_approx(
	const Vec2* points,
	const Color* C,
	const double* W,
	int length,
	const polyline_opt* opt,
	const polyline_inopt* inopt)
{
	const Point* P=(Point*)points;
	bool cap_first= inopt? !inopt->no_cap_first :true;
	bool cap_last=  inopt? !inopt->no_cap_last  :true;
	double* seg_len=inopt? inopt->segment_length: 0;

	st_anchor SA1,SA2;
	vertex_array_holder vcore;  //curve core
	vertex_array_holder vfadeo; //outer fade
	vertex_array_holder vfadei; //inner fade
	vcore.set_gl_draw_mode(GL_TRIANGLE_STRIP);
	vfadeo.set_gl_draw_mode(GL_TRIANGLE_STRIP);
	vfadei.set_gl_draw_mode(GL_TRIANGLE_STRIP);

	if( length<2)
		return;

	#define color(I)  C[inopt&&inopt->const_color?0: I]
	#define weight(I) W[inopt&&inopt->const_weight?0: I]

	for( int i=1; i<length-1; i++)
	{
		double t,r;
		determine_t_r(weight(i),t,r);
		if ( opt && opt->feather && !opt->no_feather_at_core)
			r*=opt->feathering;
		Point V=P[i]-P[i-1];
		V.perpen();
		V.normalize();
		Point F=V*r;
		V*=t;
		vcore.push( P[i]+V,  color(i));
		vcore.push( P[i]-V,  color(i));
		vfadeo.push( P[i]+V, color(i));
		vfadeo.push( P[i]+V+F, color(i), 1);
		vfadei.push( P[i]-V,   color(i));
		vfadei.push( P[i]-V-F, color(i), 1);
	}
	Point P_las,P_fir;
	Color C_las,C_fir;
	double W_las,W_fir;
	poly_point_inter( P,C,W,inopt, P_las,C_las,W_las, length-2, 0.5);
	{
		double t,r;
		determine_t_r(W_las,t,r);
		if ( opt && opt->feather && !opt->no_feather_at_core)
			r*=opt->feathering;
		Point V=P[length-1]-P[length-2];
		V.perpen();
		V.normalize();
		Point F=V*r;
		V*=t;
		vcore.push( P_las+V, C_las);
		vcore.push( P_las-V, C_las);
		vfadeo.push( P_las+V, C_las);
		vfadeo.push( P_las+V+F, C_las, 1);
		vfadei.push( P_las-V, C_las);
		vfadei.push( P_las-V-F, C_las, 1);
	}

	//first caps
	{
		poly_point_inter( P,C,W,inopt, P_fir,C_fir,W_fir, 0, inopt&&inopt->join_first? 0.5:0.0);
		SA1.P[0] = P_fir;
		SA1.P[1] = P[1];
		SA1.C[0] = C_fir;
		SA1.C[1] = color(1);
		SA1.W[0] = W_fir;
		SA1.W[1] = weight(1);
		segment( SA1, opt, cap_first,false);
	}
	//last cap
	if( !(inopt&&inopt->join_last))
	{
		SA2.P[0] = P_las;
		SA2.P[1] = P[length-1];
		SA2.C[0] = C_las;
		SA2.C[1] = color(length-1);
		SA2.W[0] = W_las;
		SA2.W[1] = weight(length-1);
		segment( SA2, opt, false,cap_last);
	}

	#undef color
	#undef weight

	if( opt && opt->tess && opt->tess->tessellate_only && opt->tess->holder)
	{
		vertex_array_holder& holder = *(vertex_array_holder*)opt->tess->holder;
		holder.push(vcore);
		holder.push(vfadeo);
		holder.push(vfadei);
		holder.push(SA1.vah);
		holder.push(SA2.vah);
	}
	else
	{
		vcore.draw();
		vfadeo.draw();
		vfadei.draw();
		SA1.vah.draw();
		SA2.vah.draw();
	}

	if ( opt && opt->tess && opt->tess->triangulation)
	{
		vcore.draw_triangles();
		vfadeo.draw_triangles();
		vfadei.draw_triangles();
		SA1.vah.draw_triangles();
		SA2.vah.draw_triangles();
	}
}

void polyline_exact(
	const Vec2* P,
	const Color* C,
	const double* W,
	int size_of_P,
	const polyline_opt* opt,
	const polyline_inopt* inopt)
{
	bool cap_first= inopt? !inopt->no_cap_first :true;
	bool cap_last=  inopt? !inopt->no_cap_last  :true;
	bool join_first = inopt && inopt->join_first;
	bool join_last =  inopt && inopt->join_last;

	#define color(I)  C[inopt&&inopt->const_color?0: I]
	#define weight(I) W[inopt&&inopt->const_weight?0: I]

	Point mid_l, mid_n; //the last and the next mid point
	Color c_l, c_n;
	double w_l, w_n;
	{	//init for the first anchor
		poly_point_inter( (Point*)P,C,W,inopt, mid_l,c_l,w_l, 0, join_first?0.5:0);
	}

	st_anchor SA;
	if ( size_of_P == 2)
	{
		SA.P[0] = P[0];
		SA.P[1] = P[1];
		SA.C[0] = color(0);
		SA.C[1] = color(1);
		SA.W[0] = weight(0);
		SA.W[1] = weight(1);
		segment( SA, opt, cap_first, cap_last);
	}
	else
	for ( int i=1; i<size_of_P-1; i++)
	{
		if ( i==size_of_P-2 && !join_last)
			poly_point_inter( (Point*)P,C,W,inopt, mid_n,c_n,w_n, i, 1.0);
		else
			poly_point_inter( (Point*)P,C,W,inopt, mid_n,c_n,w_n, i, 0.5);

		SA.P[0]=mid_l.vec(); SA.C[0]=c_l;  SA.W[0]=w_l;
		SA.P[2]=mid_n.vec(); SA.C[2]=c_n;  SA.W[2]=w_n;

		SA.P[1]=P[i];
		SA.C[1]=color(i);
		SA.W[1]=weight(i);

		anchor( SA, opt, i==1&&cap_first, i==size_of_P-2&&cap_last);

		mid_l = mid_n;
		c_l = c_n;
		w_l = w_n;
	}
	//draw or not
	if( opt && opt->tess && opt->tess->tessellate_only && opt->tess->holder)
		(*(vertex_array_holder*)opt->tess->holder).push(SA.vah);
	else
		SA.vah.draw();
	//draw triangles
	if( opt && opt->tess && opt->tess->triangulation)
		SA.vah.draw_triangles();

	#undef color
	#undef weight
}

void polyline_range(
	const Vec2* P,
	const Color* C,
	const double* W,
	int length,
	const polyline_opt* opt,
	const polyline_inopt* in_options,
	int from, int to,
	bool approx)
{
	polyline_inopt inopt={0};
	if( in_options) inopt=*in_options;
	if( from>0) from-=1;
	inopt.join_first = from!=0;
	inopt.join_last = to!=(length-1);
	inopt.no_cap_first = inopt.no_cap_first || inopt.join_first;
	inopt.no_cap_last = inopt.no_cap_last || inopt.join_last;

	if( approx)
		polyline_approx( P+from, C+(inopt.const_color?0:from), W+(inopt.const_weight?0:from), to-from+1, opt, &inopt);
	else
		polyline_exact ( P+from, C+(inopt.const_color?0:from), W+(inopt.const_weight?0:from), to-from+1, opt, &inopt);
}

void polyline(
	const Vec2* PP,  //pointer to array of point of a polyline
	const Color* C,  //array of color
	const double* W, //array of weight
	int length, //size of the buffer P
	const polyline_opt* options, //options
	const polyline_inopt* in_options) //internal options
{
	polyline_opt   opt={0};
	polyline_inopt inopt={0};
	if( options)    opt=*options;
	if( in_options) inopt=*in_options;

	/* if( opt.fallback)
	{
		backend::polyline(PP,C[0],W[0],length,0);
		return;
	} */

	if( opt.cap >= 10)
	{
		char dec=(opt.cap/10)*10;
		if( dec==PLC_first || dec==PLC_none)
			inopt.no_cap_last=true;
		if( dec==PLC_last || dec==PLC_none)
			inopt.no_cap_first=true;
		opt.cap -= dec;
	}

	if( inopt.const_weight && W[0] < cri_segment_approx)
	{
		polyline_exact(PP,C,W,length,&opt,&inopt);
		return;
	}

	const Point* P=(Point*)PP;
	int A=0,B=0;
	bool on=false;
	for( int i=1; i<length-1; i++)
	{
		Point V1=P[i]-P[i-1];
		Point V2=P[i+1]-P[i];
		double len=0.0;
		if( inopt.segment_length)
		{
			V1 /= inopt.segment_length[i];
			V2 /= inopt.segment_length[i+1];
			len += (inopt.segment_length[i]+inopt.segment_length[i+1])*0.5;
		}
		else
		{
			len += V1.normalize()*0.5;
			len += V2.normalize()*0.5;
		}
		double costho = V1.x*V2.x+V1.y*V2.y;
		//double angle = acos(costho)*180/vaser_pi;
		const double cos_a = cos(15*vaser_pi/180);
		const double cos_b = cos(10*vaser_pi/180);
		const double cos_c = cos(25*vaser_pi/180);
		double weight = W[inopt.const_weight?0:i];
		bool approx = false;
		if( (weight<7 && costho>cos_a) ||
			(costho>cos_b) || //when the angle difference at an anchor is smaller than a critical degree, do polyline approximation
			(len<weight && costho>cos_c) ) //when vector length is smaller than weight, do approximation
			approx = true;
		if( approx && !on)
		{
			A=i; if( A==1) A=0;
			on=true;
			if( A>1)
				polyline_range(PP,C,W,length,&opt,&inopt,B,A,false);
		}
		else if( !approx && on)
		{
			B=i;
			on=false;
			polyline_range(PP,C,W,length,&opt,&inopt,A,B,true);
		}
	}
	if( on && B<length-1)
	{
		B=length-1;
		polyline_range(PP,C,W,length,&opt,&inopt,A,B,true);
	}
	else if( !on && A<length-1)
	{
		A=length-1;
		polyline_range(PP,C,W,length,&opt,&inopt,B,A,false);
	}
}

#undef push_quad
#undef push_quadf

} //sub namespace VASErin

//export implementations

void polyline( const Vec2* P, const Color* C, const double* W, int length, const polyline_opt* opt)
{
	VASErin::polyline(P,C,W,length,opt,0);
}
void polyline( const Vec2* P, Color C, double W, int length, const polyline_opt* opt) //constant color and weight
{
	VASErin::polyline_inopt inopt={0};
	inopt.const_color=true;
	inopt.const_weight=true;
	VASErin::polyline(P,&C,&W,length,opt,&inopt);
}
void polyline( const Vec2* P, const Color* C, double W, int length, const polyline_opt* opt) //constant weight
{
	VASErin::polyline_inopt inopt={0};
	inopt.const_weight=true;
	VASErin::polyline(P,C,&W,length,opt,&inopt);
}
void polyline( const Vec2* P, Color C, const double* W, int length, const polyline_opt* opt) //constant color
{
	VASErin::polyline_inopt inopt={0};
	inopt.const_color=true;
	VASErin::polyline(P,&C,W,length,opt,&inopt);
}

void segment(  Vec2 P1, Vec2 P2, Color C1, Color C2, double W1, double W2, const polyline_opt* options)
{
	Vec2   AP[2];
	Color  AC[2];
	double AW[2];
		AP[0] = P1; AC[0] = C1; AW[0] = W1;
		AP[1] = P2; AC[1] = C2; AW[1] = W2;
	polyline( AP, AC, AW, 2, options);
}
void segment(  Vec2 P1, Vec2 P2, Color C, double W, const polyline_opt* options) //constant color and weight
{
	Vec2   AP[2];
		AP[0] = P1;
		AP[1] = P2;
	polyline( AP, C, W, 2, options);
}
void segment(  Vec2 P1, Vec2 P2, Color C1, Color C2, double W, const polyline_opt* options) //constant weight
{
	Vec2   AP[2];
	Color  AC[2];
	double AW[2];
		AP[0] = P1; AC[0] = C1; AW[0] = W;
		AP[1] = P2; AC[1] = C2; AW[1] = W;
	polyline( AP, AC, AW, 2, options);
}
void segment(  Vec2 P1, Vec2 P2, Color C, double W1, double W2, const polyline_opt* options) //const color
{
	Vec2   AP[2];
	Color  AC[2];
	double AW[2];
		AP[0] = P1; AC[0] = C; AW[0] = W1;
		AP[1] = P2; AC[1] = C; AW[1] = W2;
	polyline( AP, AC, AW, 2, options);
}
