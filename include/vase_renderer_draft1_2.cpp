#ifndef VASE_RENDERER_DRAFT1_2_CPP
#define VASE_RENDERER_DRAFT1_2_CPP

#include "vase_renderer_draft1_2.h"

#include "Color.h"
#include "vector_operations.h"
#include "vertex_array_holder.h"

#include <math.h>

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
 * 4.  [nearly solved]when 2 segments make < 5 degrees,,
 * 4.1 when 2 segments make exactly 0 degree,,
 * 5.  when a segment is shorter than its own width,,
 */

static void determine_t_r ( double w, double& t, double& R)
{
	//efficiency: can cache one set of w,t,R values
	// i.e. when a polyline is of uniform thickness, the same w is passed in repeatedly
	double f=w-static_cast<int>(w);
	
	/*   */if ( w>=0.0 && w<1.0) {
		t=0.05; R=0.48+0.32*f;
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
	
	//PPI correction
	double PPI_correction = vaserend_standard_PPI / vaserend_actual_PPI;
	const double up_bound = 1.6; //max value of w to receive correction
	const double start_falloff = 1.0;
	if ( w>0.0 && w<up_bound)
	{	//here we gracefully apply the correction
		// so that the effect of correction diminishes starting from w=start_falloff
		//   and completely disappears when w=up_bound
		double correction = 1.0 + (PPI_correction-1.0)*(up_bound-w)/(up_bound-start_falloff);
		t *= PPI_correction;
		R *= PPI_correction;
	}
}
static void make_T_R_C( const Point& P1, const Point& P2, Point* T, Point* R, Point* C,
				double w, const polyline_opt& opt,
				double* rr, double* tt)
{
	double t=1.0,r=0.0;
	Point DP=P2-P1;
	
	//calculate t,r
	determine_t_r( w,t,r);
	if ( opt.feather && !opt.no_feather_at_core)
		r *= opt.feathering;
	
	//output t,r
	if (tt) *tt = t;
	if (rr) *rr = r;
	
	//calculate T,R,C
	DP.normalize();
	if (C) *C = DP;
	DP.perpen();
	if (T) *T = DP*t;
	if (R) *R = DP*r;
}
static void same_side_of_line( Point& V, const Point& ref, const Point& a, const Point& b)
{
	double sign1 = Point::signed_area( a+ref,a,b);
	double sign2 = Point::signed_area( a+V,  a,b);
	if ( (sign1>=0) != (sign2>=0))
	{
		V.opposite();
	}
}
static Point plus_minus( const Point& a, const Point& b, bool plus)
{
	if (plus) return a+b;
	else return a-b;
}
static Point plus_minus( const Point& a, bool plus)
{
	if (plus) return a;
	else return -a;
}

struct _st_polyline
{
	//for djoint==LJ_miter
	Point vP; //vector to intersection point
	Point vR; //fading vector at sharp end
		//all vP,vR must point to the same side of the polyline
	
	//for djoint==LJ_bevel
	Point T; //core thickness of a line
	Point R; //fading edge of a line
	Point bR; //out stepping vector, same direction as cap
	Point T1,R1; //alternate vectors, same direction as T21
		//all T,R,T1,R1 must point to the same side of the polyline 
	bool T_outward_at_opp;  //]if true, opposite T to get the outward vector
	bool T1_outward_at_opp; //]
	bool bevel_at_positive;
	
	//for djoint==LJ_round
	float t,r;
	
	//for degeneration case
	bool degenT; //core degenerate
	bool degenR; //fade degenerate
	bool pre_full; //draw the preceding segment in full
	Point PT,PR;
	bool R_full_degen;
	
	char djoint; //determined joint
			// e.g. originally a joint is LJ_miter. but it is smaller than critical angle,
			//   should then set djoint to LJ_bevel
	#define LJ_end   101 //used privately by polyline
};

static void inner_arc_strip( vertex_array_holder& hold, const Point& P, //P: center
		const Color& C, const Color& C2,
		float dangle, float angle1, float angle2,
		float r, float r2, bool ignor_ends,
		Point* apparent_P)	//(apparent center) center of fan
{
	const double& m_pi = vaserend_pi;
	
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
				hold.push( Point(P.x+x*r,P.y-y*r), C);
				if ( !apparent_P)
					hold.push( Point(P.x+x*r2,P.y-y*r2), C2);
				else
					hold.push( *apparent_P, C2);
				
				if ( i>100) {
					printf("trapped in loop: inc,ig_end "
						"angle1=%.2f, angle2=%.2f, dangle=%.2f\n",
						angle1, angle2, dangle);
					break;
				}
			}
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
				hold.push( Point(P.x+x*r,P.y-y*r), C);
				if ( !apparent_P)
					hold.push( Point(P.x+x*r2,P.y-y*r2), C2);
				else
					hold.push( *apparent_P, C2);
				
				if ( a>=angle2)
					break;
				
				if ( i>100) {
					printf("trapped in loop: inc,end "
						"angle1=%.2f, angle2=%.2f, dangle=%.2f\n",
						angle1, angle2, dangle);
					break;
				}
			}
			//printf( "steps=%d ",i);
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
				hold.push( Point(P.x+x*r,P.y-y*r), C);
				if ( !apparent_P)
					hold.push( Point(P.x+x*r2,P.y-y*r2), C2);
				else
					hold.push( *apparent_P, C2);
				
				if ( i>100) {
					printf("trapped in loop: dec,ig_end "
						"angle1=%.2f, angle2=%.2f, dangle=%.2f\n",
						angle1, angle2, dangle);
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
				hold.push( Point(P.x+x*r,P.y-y*r), C);
				if ( !apparent_P)
					hold.push( Point(P.x+x*r2,P.y-y*r2), C2);
				else
					hold.push( *apparent_P, C2);
				
				if ( a<=angle2)
					break;
				
				if ( i>100) {
					printf("trapped in loop: dec,end "
						"angle1=%.2f, angle2=%.2f, dangle=%.2f\n",
						angle1, angle2, dangle);
					break;
				}
			}
		}
	}
}
static void vectors_to_arc( vertex_array_holder& hold, const Point& P,
		const Color& C, const Color& C2,
		Point A, Point B, float dangle, float r, float r2, bool ignor_ends,
		Point* apparent_P)
{
	const double& m_pi = vaserend_pi;
	A *= 1/r;
	B *= 1/r;
	float angle1 = acos(A.x);
	float angle2 = acos(B.x);
	if ( A.y>0){ angle1=2*m_pi-angle1;}
	if ( B.y>0){ angle2=2*m_pi-angle2;}
	
	//printf( "steps=%d ",int((angle2-angle1)/den*r));

	inner_arc_strip( hold, P, C,C2, dangle,angle1,angle2, r,r2, ignor_ends, apparent_P);
}
inline static float get_LJ_round_dangle(float t, float r)
{
	float dangle;
	if ( t<=1.9f) //w<=5.0
		dangle = 2.0f/(t+r);
	else
		dangle = 3.0f/(t+r);
	return dangle;
}
static void annotate( const Point& P, Color cc, int I=-1)
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
static void annotate( const Point& P)
{
	Color cc;
	annotate(P,cc);
}
static void printpoint( const Point& P, const char* name)
{
	printf("%s(%.4f,%.4f) ",name,P.x,P.y);
	fflush(stdout);
}
static void draw_vector( const Point& P, const Point& V, const char* name)
{
	Point P2 = P+V*10;
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
static bool quad_is_reflexed( const Point& P0, const Point& P1, const Point& P2, const Point& P3)
{
	//points:
	//   1------3
	//  /      /
	// 0------2
	// vector 01 parallel to 23
	
	return Point::distance_squared(P1,P3) + Point::distance_squared(P0,P2)
		> Point::distance_squared(P0,P3) + Point::distance_squared(P1,P2);
}
static void push_quad_safe( vertex_array_holder& core,
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
}

int triangle_knife_cut( const Point& kn1, const Point& kn2, const Point& kn_out, //knife
		Point& p1, Point& p2, Point& p3, Point& p4, //]will modify these for output
		Color& c1, Color& c2, Color& c3, Color& c4) //]
{	//return number of points cut away
	
	bool std_sign = Point::signed_area( kn1,kn2,kn_out) > 0;
	bool s1 = Point::signed_area( kn1,kn2,p1)>0 == std_sign; //true means this point should be cut
	bool s2 = Point::signed_area( kn1,kn2,p2)>0 == std_sign;
	bool s3 = Point::signed_area( kn1,kn2,p3)>0 == std_sign;
	int sums = int(s1)+int(s2)+int(s3);
	
	if ( sums == 0)
	{	//all 3 points are retained
		return 0;
	}
	else if ( sums == 3)
	{	//all 3 are cut away
		return 3;
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
			outp= p1;  outC= c1;
			ip1 = p2;  iC1 = c2;
			ip2 = p3;  iC2 = c3;
		} else if ( s2) {
			outp= p2;  outC= c2;
			ip1 = p1;  iC1 = c1;
			ip2 = p3;  iC2 = c3;
		} else if ( s3) {
			outp= p3;  outC= c3;
			ip1 = p1;  iC1 = c1;
			ip2 = p2;  iC2 = c2;
		}
		
		Point interP1,interP2;
		Color interC1,interC2;
		double ble1, ble2;
		Point::intersect( kn1,kn2, ip1,outp, interP1, 0,&ble1);
		Point::intersect( kn1,kn2, ip2,outp, interP2, 0,&ble2);
		interC1 = Color_between( iC1, outC, ble1);
		interC2 = Color_between( iC2, outC, ble2);
		//ip2 first gives a polygon
		//ip1 first gives a triangle strip
		
		if ( sums == 1) {
			p1 = ip1;      c1 = iC1;
			p2 = ip2;      c2 = iC2;
			p3 = interP1;  c3 = interC1;
			p4 = interP2;  c4 = interC2;
			//one point is cut away
			return 1;
		} else if ( sums == 2) {
			p1 = outp;     c1 = outC;
			p2 = interP1;  c2 = interC1;
			p3 = interP2;  c3 = interC2;
			//two points are cut away
			return 2;
		}
	}
}
static void vah_knife_cut( vertex_array_holder& core,
		const Point& kn1, const Point& kn2, const Point& kn_out)
{
	Point p1,p2,p3,p4;
	Color c1,c2,c3,c4;
	for ( int i=0; i<core.get_count(); i+=3)
	{
		p1 = core.get(i);
		p2 = core.get(i+1);
		p3 = core.get(i+2);
		c1 = core.get_color(i);
		c2 = core.get_color(i+1);
		c3 = core.get_color(i+2);
		
		int result = triangle_knife_cut( kn1,kn2,kn_out,
					p1,p2,p3,p4,
					c1,c2,c3,c4);
		
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
			core.replace(i,p1,c1);
			core.replace(i+1,p2,c2);
			core.replace(i+2,p3,c3);
			
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
				core.replace( a3, p4,c4);
			}
		break;
		
		}
	}
}

void polyline_late( Vec2* P, Color* C, _st_polyline* SL, int size_of_P, Point cap1, Point cap2)
{
	vertex_array_holder core;
	vertex_array_holder fade[2];
	vertex_array_holder cap[2];
	vertex_array_holder tris;
	
	core.set_gl_draw_mode(GL_TRIANGLE_STRIP);
	fade[0].set_gl_draw_mode(GL_TRIANGLE_STRIP);
	fade[1].set_gl_draw_mode(GL_TRIANGLE_STRIP);
	cap[0].set_gl_draw_mode(GL_TRIANGLES);
	cap[1].set_gl_draw_mode(GL_TRIANGLES);
	tris.set_gl_draw_mode(GL_TRIANGLES);
	
	bool K=0;
	for ( int i=0; i<size_of_P; i++)
	{
		Point P_cur, P_las, P_nxt;
		
		P_cur = P[i];  //current point
		bool Q = SL[i].bevel_at_positive; //current
		
		if ( i == 0) P_cur -= cap1;
		else P_las = P[i-1]; //last point
		
		if ( i == size_of_P-1) P_cur -= cap2;
		else P_nxt = P[i+1]; //next point
		
		if ( i == 1)
			P_las -= cap1;
		if ( i == size_of_P-2)
			P_nxt -= cap2;
		
		bool degen_nxt = 0;
		bool degen_las = 0;
		bool degen_cur = SL[i].degenT;
		if ( i < size_of_P-2)
			degen_nxt = SL[i+1].degenT && ! SL[i+1].pre_full;
		if ( i > 1)
			degen_las = SL[i-1].degenT && SL[i-1].pre_full;
		
		bool degen_nxtR = 0;
		bool degen_lasR = 0;
		if ( i < size_of_P-2)
			degen_nxtR = SL[i+1].degenR && ! SL[i+1].pre_full;
		if ( i > 1)
			degen_lasR = SL[i-1].degenR && SL[i-1].pre_full;
		
		if ( degen_cur)
		{	//degen line core
			
			/* //see cap03_ _.png
			Point P0,P1,P2,P6,P7,P8;
			if ( SL[i].pre_full)
			{
				P0 = plus_minus(P_las,SL[i-1].T, K);
				P1 = plus_minus(P_las,SL[i-1].T, !K);
				P2 = plus_minus(P_cur,SL[i].T1, K);
				P6 = plus_minus(P_nxt,SL[i+1].T, Q);
				P7 = plus_minus(P_cur,SL[i].T, K);
				P8 = plus_minus(SL[i].T1, K);
			}
			else
			{
				P0 = plus_minus(P_nxt,SL[i+1].T, K);
				P1 = plus_minus(P_nxt,SL[i+1].T, !K);
				P2 = plus_minus(P_cur,SL[i].T, K);
				P6 = plus_minus(P_las,SL[i-1].T, Q);
				P7 = plus_minus(P_cur,SL[i].T1, K);
				P8 = plus_minus(SL[i].T, K);
			}*/
			
			Point P10 = SL[i].PT;
			
			switch( SL[i].djoint)
			{
			case LJ_miter:
			{
				Point interP = plus_minus( P_cur,SL[i].vP, Q);
				if ( K != Q)
				{
					core.push( P10, C[i]);
					core.push( interP, C[i]);
				}
				else
				{
					core.push( interP, C[i]);
				}
			} break;
			
			case LJ_bevel:
			{
				Point P2, P7;
				P2 = plus_minus(P_cur,SL[i].T, !SL[i].T_outward_at_opp);
				P7 = plus_minus(P_cur,SL[i].T1, !SL[i].T1_outward_at_opp);
				if ( K != Q)
				{	//0 1 | 10 7 10 2 | 10 6
					core.push( P10,C[i]);
					core.push( P7,C[i]);
					core.push( P10,C[i]);
					core.push( P2,C[i]);
				}
				else
				{
					core.push( P7,C[i]);
					core.push( P10,C[i]);
					core.push( P2,C[i]);
					core.push( P10,C[i]);
				}
			} break;
			
			case LJ_round:
			{
				Point P2, P7;
				P2 = plus_minus(P_cur,SL[i].T, !SL[i].T_outward_at_opp);
				P7 = plus_minus(P_cur,SL[i].T1, !SL[i].T1_outward_at_opp);
				
				float dangle = get_LJ_round_dangle(SL[i].t,SL[i].r);
				if ( K != Q)
				{
					core.push( P10,C[i]);
					//circular fan by degenerated triangles
					vectors_to_arc( core, P_cur, C[i], C[i],
					P7-P_cur, P2-P_cur,
					dangle, SL[i].t, 0.0f, false, &P10);
					core.push( P2,C[i]);
					
					//Point apparent_P = P_a*1.0 + P_b*0.0 + P_cur;
					//Point apparent_P = P7; //may have caused a tiny artifact
				}
				else
				{
					//circular fan by degenerated triangles
					vectors_to_arc( core, P_cur, C[i], C[i],
					P7-P_cur, P2-P_cur,
					dangle, SL[i].t, 0.0f, false, &P10);
					K = !K;
				}
			} break;
			
			}
		}
		else if ( degen_nxt | degen_las)
		{
			int ia;
			if ( degen_nxt)
				ia = i+1;
			else
				ia = i-1;
			
			Point& P_nxl = ia==i+1? P_nxt:P_las;
			bool Q_nxl = SL[ia].bevel_at_positive;
			
			Point P6;
			P6 = plus_minus(P_cur,SL[i].T, Q_nxl);
			
			switch( SL[i].djoint)
			{
			case LJ_end:
				//10 6
				if ( Q_nxl)
				{
					core.push(SL[ia].PT,C[ia]);
					core.push(P6,C[i]);
				}
				else
				{
					core.push(P6,C[i]);
					core.push(SL[ia].PT,C[ia]);
				}
			break;
			
			case LJ_bevel:
			case LJ_miter:
			case LJ_round:
			break;
			}
		}
		else
		{
			if ( i==0)
			{	//debug only
				draw_vector(P_cur,SL[i].T,"T");
			}
			//normal line core
			switch (SL[i].djoint)
			{
				case LJ_end:
					core.push(plus_minus(P_cur,SL[i].T, K), C[i],false);
					core.push(plus_minus(P_cur,SL[i].T, !K), C[i],false);
				break;
				
				case LJ_miter:
					core.push( plus_minus(P_cur,SL[i].vP, K),  C[i]);
					core.push( plus_minus(P_cur,SL[i].vP, !K), C[i]);
				break;
				
				case LJ_bevel:
				{
					Point T = plus_minus( P_cur, SL[i].T, !SL[i].T_outward_at_opp);
					Point T1 = plus_minus( P_cur, SL[i].T1, !SL[i].T1_outward_at_opp);
					Point neg_interP = plus_minus(P_cur,SL[i].vP, !Q);
					
					if ( K != Q) {
						core.push( neg_interP, C[i]);
						core.push( T1, C[i]);
						core.push( neg_interP, C[i]);//degenerated triangle
						core.push( T, C[i]);
					} else {
						core.push( T1, C[i]);
						core.push( neg_interP, C[i]);
						core.push( T, C[i]);
						K = !K;
					}
				} break;
				
				case LJ_round:
				{
					Point T = plus_minus( P_cur, SL[i].T, !SL[i].T_outward_at_opp);
					Point T1 = plus_minus( P_cur, SL[i].T1, !SL[i].T1_outward_at_opp);
					Point neg_interP = plus_minus(P_cur,SL[i].vP, !Q);
					
					//preceding points
					if ( K != Q) {
						core.push( neg_interP, C[i]);
						core.push( T1, C[i]);
						core.push( neg_interP, C[i]);//degenerated triangle
					} else {
						core.push( T1, C[i]);
						core.push( neg_interP, C[i]);
					}

					//circular fan by degenerated triangles
					float dangle = get_LJ_round_dangle(SL[i].t,SL[i].r);
					Point apparent_P = neg_interP;
					
					vectors_to_arc( core, P_cur, C[i], C[i],
					plus_minus(SL[i].T1, !SL[i].T1_outward_at_opp),
					plus_minus(SL[i].T, !SL[i].T_outward_at_opp),
					dangle, SL[i].t, 0.0f, true, &apparent_P);

					//succeeding point
					core.push( T, C[i]);
					
					if ( K == Q)
						K = !K;
				} break;
			}	//*/
			
			if ( SL[i].degenR && !SL[i].pre_full)
			{	//to fix an incomplete welding edge
				/*annotate( core.get_relative_end(), C[i], 0);
				if ( K != Q)
					annotate( plus_minus(P_cur,SL[i].vP, K), C[i],1);
				else
					annotate( plus_minus(P_cur,SL[i].vP, !K), C[i],2);
				*/
				Color cbt = Color_between(C[i],C[i+1], 0.3f);
				switch (SL[i].djoint)
				{
				case LJ_miter:
					if ( K != Q) {
						core.push( SL[i].PT, cbt);
						K = !K;
					} else {
						core.push( plus_minus(P_cur,SL[i].vP, K), C[i]);
						core.push( SL[i].PT, cbt);
					}
				break;
				case LJ_bevel:
				case LJ_round:
					//push and repeat the last point _before_ push
					int a = core.get_count()-1;
					core.push( SL[i].PT, cbt);
					int b = core.push( Point(), C[i]);
					core.move( b,a);
				break;
				}
			}
		}
		
		//fade
		for ( int E=0; E<=1; E++)
		{
			if ( SL[i].degenR && Q != bool(E))
			{
				if ( SL[i].degenT)
				{	// SL[i].degenR && SL[i].degenT
				
					if ( !SL[i].pre_full) fade[E].jump();
					
					fade[E].push(SL[i].PT,C[i]);
					fade[E].push(SL[i].PR,C[i],true);
					
					if ( SL[i].pre_full) fade[E].jump();
				}
				else if ( !SL[i].pre_full)
				{
					Point P2 = plus_minus(P_cur,SL[i].vP, !K);
					Point P3 = SL[i].PT;
					Point P4 = P_las - plus_minus(SL[i-1].T, !SL[i-1].T_outward_at_opp);
					Point cen= (P3+P4)*0.5;
					
					Color cbt = Color_between(C[i],C[i+1], 0.3f);
					
					fade[E].jump();
					fade[E].push(SL[i].PT,cbt);
					fade[E].push(SL[i].PR,cbt,true);
					
					tris.push( P2, C[i]);	//cur
					tris.push( P3, cbt);	//nxt
					tris.push( cen,cbt, true);
					
					tris.push( P2, C[i]);	//cur
					tris.push( P4, C[i-1]); //las
					tris.push( cen,C[i-1], true);
				}
				else
				{	// SL[i].degenR && ! SL[i].degenT
				
					Point P0,P1,P2,P3, P10,P11;
					P0 = SL[i].PR;
					P2 = P_cur-SL[i].vP;
					
					//see cap04.png
					//~ P1 = SL[i].PT;
					//~ if ( SL[i].pre_full)
					//~ {
						//~ P3 = plus_minus(P_nxt,SL[i+1].T, E);
						//~ P10= plus_minus(P_las,SL[i-1].T, E);
						//~ P11= plus_minus(P_las,SL[i-1].T+SL[i-1].R, E);
					//~ }
					//~ else
					//~ {
						//~ P3 = plus_minus(P_las,SL[i-1].T, E);
						//~ P10= plus_minus(P_nxt,SL[i+1].T, E);
						//~ P11= plus_minus(P_nxt,SL[i+1].T+SL[i+1].R, E);
					//~ }
					
					fade[E].push(P2,C[i]);
					fade[E].push(P0,C[i],true);
				}
			}
			else if ( degen_lasR && SL[i-1].bevel_at_positive != bool(E))
			{
				int ia;
				if ( degen_nxtR)
					ia = i+1;
				else
					ia = i-1;
				
				switch (SL[i].djoint)
				{
					case LJ_end:
						fade[E].push( plus_minus(P_cur,SL[i].T, E), C[i]);
						fade[E].push( SL[ia].PR, C[i], true);
					break;
				}
			}
			else if ( degen_nxtR && SL[i+1].bevel_at_positive != bool(E))
			{
				switch (SL[i].djoint)
				{
					case LJ_end:
						fade[E].push( plus_minus(P_cur,SL[i].T, E), C[i]);
						fade[E].push( SL[i+1].PR, C[i], true);
					break;
				}
			}
			else
			{	//normal fade
				switch (SL[i].djoint)
				{
					case LJ_end:
						fade[E].push( plus_minus(P_cur,SL[i].T, E), C[i]);
						fade[E].push( plus_minus(P_cur,SL[i].T+SL[i].R, E), C[i],true);
					break;
					
					case LJ_miter:
						fade[E].push( plus_minus(P_cur,SL[i].vP, E), C[i]);
						fade[E].push( plus_minus(P_cur,SL[i].vP+SL[i].vR, E), C[i],true);
					break;
					
					case LJ_bevel:
					if ( Q == bool(E)) {
						fade[E].push( plus_minus(P_cur,SL[i].T1, E), C[i]);
						fade[E].push( plus_minus(P_cur,SL[i].T1+SL[i].R1, E) + SL[i-1].bR,
								C[i],true);
						fade[E].push( plus_minus(P_cur,SL[i].T, E), C[i]);
						fade[E].push( plus_minus(P_cur,SL[i].T+SL[i].R, E) - SL[i+1].bR,
								C[i],true);
					} else {
						fade[E].push( plus_minus(P_cur,SL[i].vP, E), C[i]);
						fade[E].push( plus_minus(P_cur,SL[i].vP+SL[i].vR, E), C[i],true);
					}
					break;
					
					case LJ_round:
					{
					float dangle = get_LJ_round_dangle(SL[i].t,SL[i].r);
					Color C2=C[i]; C2.a=0.0;
					if ( Q == bool(E)) {
						vectors_to_arc( fade[E], P_cur, C[i], C2,
						plus_minus(SL[i].T1, E), plus_minus(SL[i].T, E),
						dangle, SL[i].t, SL[i].t+SL[i].r, false, 0);
					} else {
						//same as miter
						fade[E].push( plus_minus(P_cur,SL[i].vP, E), C[i]);
						fade[E].push( plus_minus(P_cur,SL[i].vP+SL[i].vR, E), C[i],true);
					}
					} break;
				}
			}
		}	//*/
	}
	
	for ( int i=0,k=0; k<=1; i=size_of_P-1, k++)
	{	//caps
		Point P_cur = P[i];
		bool degen_nxt=0, degen_las=0;
		if ( k == 0)
		{
			P_cur -= cap1;
		}
		if ( k == 1)
		{
			P_cur -= cap2;
		}
		//TODO: to fix overdraw in cap
		
		Point& cur_cap = i==0? cap1:cap2;
		if ( cur_cap.non_zero()) //save some degenerated points
		{
			Point P0,P1,P2,P3,P4,P5,P6;
			
			P0 = P_cur+SL[i].T+SL[i].R;
			P1 = P_cur+SL[i].T+SL[i].R+cur_cap;
			P2 = P_cur+SL[i].T;
			P3 = P_cur-SL[i].T-SL[i].R+cur_cap;
			P4 = P_cur-SL[i].T;
			P5 = P_cur-SL[i].T-SL[i].R;
			
			cap[k].push( P0, C[i],true);
			cap[k].push( P1, C[i],true);
			cap[k].push( P2, C[i]);
			
					cap[k].push( P1, C[i],true);
				cap[k].push( P2, C[i]);
			cap[k].push( P3, C[i],true);
			
					cap[k].push( P2, C[i]);
				cap[k].push( P3, C[i],true);
			cap[k].push( P4, C[i]);
			
					cap[k].push( P3, C[i],true);
				cap[k].push( P4, C[i]);
			cap[k].push( P5, C[i],true);
			//say if you want to use triangle strip,
			//  just push P0~ P5 in sequence
			
			if ( SL[1].degenR && SL[1].R_full_degen)
			{
				if ((k==0 && !SL[1].pre_full) ||
					(k==1 && SL[1].pre_full))
				{
					vah_knife_cut( cap[k], SL[1].PT, SL[1].PR, P3);
					/*annotate(SL[1].PT,C[i],0);
					annotate(SL[1].PR);
					annotate(P3);*/
				}
			}
		}
	}
	
	//actually this is not the only place where .draw() occurs
	// ,when a vertex_array_holder's internal array is full
	// ,it will draw and flush
	core   .draw();
	fade[0].draw();
	fade[1].draw();
	cap[0] .draw();
	cap[1] .draw();
	tris   .draw();
}

void anchor( Vec2* P, Color* C, double* weight, int size_of_P, polyline_opt* options, 
		bool cap_first, bool cap_last)
{
	if ( !P || !C || !weight) return;
	
	polyline_opt opt;
	if ( options)
		opt = (*options);
	
	//const double critical_angle=11.6538;
	/*critical angle in degrees where a miter is force into bevel
	 * it is _similar_ to cairo_set_miter_limit ()
	 * but cairo works with ratio while we work with included angle*/
	
	const double cos_cri_angle=0.979386; //cos(critical_angle)
	
	Point T1,T2,T21,T31;		//]these are for calculations in early stage
	Point R1,R2,R21,R31;		//]
	
	Point cap_start, cap_end;			//]these 2 lines stores information
	_st_polyline SL[3];
	
	for ( int i=0; i<3; i++)
	{
		if ( weight[i]>=0.0 && weight[i]<1.0)
		{
			double f=weight[i]-static_cast<int>(weight[i]);
			C[i].a *=f;
		}
	}
	
	{	int i=0;
	
		Point cap1;
		double r,t;
		make_T_R_C( Point(P[i]), Point(P[i+1]), &T2,&R2,&cap1, weight[i],opt, &r,&t);
		make_T_R_C( Point(P[i]), Point(P[i+1]), &T31,&R31,0, weight[i+1],opt, 0,0);
			Point::anchor_outward(R2, P[i+1],P[i+2], 0/*debug only inward_first->value()*/);
				T2.follow_signs(R2);
		
		SL[i].bR=cap1;
		
		if ( cap_first)
		{
			cap1.opposite(); if ( opt.feather && !opt.no_feather_at_cap)
			cap1*=opt.feathering;
			cap_start = cap1;
		}
		
		SL[i].djoint=LJ_end;
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
		make_T_R_C( P[i-1],P[i], 0,0,&cap2,weight[i],opt, 0,0);
		if ( opt.feather && !opt.no_feather_at_cap)
			cap2*=opt.feathering;
		cap_end = cap2;
	}
	
	{	int i=1;
	
		double r,t;
		Point P_cur = P[i]; //current point //to avoid calling constructor repeatedly
		Point P_nxt = P[i+1]; //next point
			P_nxt -= cap_end;
		Point P_las = P[i-1]; //last point
			P_las -= cap_start;
		
		{
		Point bR;
		make_T_R_C( P_las, P_cur,  &T1,&R1, 0, weight[i-1],opt,0,0);
		make_T_R_C( P_las, P_cur, &T21,&R21,0, weight[i],opt,  0,0);
		
		make_T_R_C( P_cur, P_nxt,  &T2,&R2,&bR, weight[i],opt, &r,&t);
		make_T_R_C( P_cur, P_nxt, &T31,&R31,0, weight[i+1],opt, 0,0);
		
		//TODO: (optional) weight compensation when segment is < 1.0px and exactly hori/ vertical
		
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
			
			if ( cos_tho < 0)
			{	//when greater than 90 degrees
				SL[i-1].bR.opposite();
				SL[i+1].bR.opposite();
				//to solve an overdraw in bevel joint
			}
			
			//make intersections
			Point PR1,PR2, PT1,PT2;
			Point::anchor_inward( T1, P_cur,P_nxt);
				R1.follow_signs(T1);
			Point::anchor_inward( T21, P_cur,P_nxt);
				R21.follow_signs(T21);
			Point::anchor_inward( T2, P_cur,P_las);
				R2.follow_signs(T2);
			Point::anchor_inward( T31, P_cur,P_las);
				R31.follow_signs(T31);
			
			int result1r = Point::intersect( P_nxt-T31-R31, P_nxt+T31+R31,
						P_las+T1+R1, P_cur+T21+R21, //knife1
						PR1); //fade
			int result2r = Point::intersect( P_las-T1-R1, P_las+T1+R1,
						P_nxt+T31+R31, P_cur+T2+R2, //knife2
						PR2);
			bool is_result1r = result1r == 1;
			bool is_result2r = result2r == 1;
			//
			int result1t = Point::intersect( P_nxt-T31, P_nxt+T31,
						P_las+T1, P_cur+T21, //knife1_a
						PT1); //core
			int result2t = Point::intersect( P_las-T1, P_las+T1,
						P_nxt+T31, P_cur+T2, //knife2_a
						PT2);
			bool is_result1t = result1t == 1;
			bool is_result2t = result2t == 1;
			//
			if ( (is_result1r | is_result2r) && !zero_degree)
			{	//fade degeneration
				SL[i].degenR=true;
				SL[i].PT = is_result1r? PT1:PT2; //this is is_result1r!!
				SL[i].PR = is_result1r? PR1:PR2;
				SL[i].pre_full = is_result1r;
				SL[i].R_full_degen = false;
				
				Point P_nxt = P[i+1]; //override that in the parent scope
				Point P_las = P[i-1];
				Point PR;
				int result2;
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
			
			if ( (is_result1t | is_result2t) && !zero_degree)
			{	//core degeneration
				SL[i].degenT=true;
				SL[i].pre_full=is_result1t;
				SL[i].PT = is_result1t? PT1:PT2;
			}
			
			//make joint
			SL[i].djoint = opt.joint;
			if ( opt.joint == LJ_miter)
				if ( cos_tho >= cos_cri_angle)
					SL[i].djoint=LJ_bevel;
			
			T1.opposite();  //outward
			T21.opposite(); //outward
				SL[i].T1.follow_signs(T21);
				SL[i].R1.follow_signs(T21);
			T2.opposite();  //outward
				SL[i].T.follow_signs(T2);
				SL[i].R.follow_signs(T2);
			T31.opposite(); //outward
			
			{ //must do intersection
				Point interP, vP;
				int result3 = Point::intersect( P_las+T1, P_cur+T21,
							P_nxt+T31, P_cur+T2,
							interP);
				
				if ( result3) {
					vP = interP - P_cur;
					SL[i].vP=vP;
					SL[i].vR=vP*(r/t);
				} else {
					SL[i].vP=SL[i].T;
					SL[i].vR=SL[i].R;
					printf( "intersection failed: cos(angle)=%.4f, angle=%.4f(degree)\n",
						cos_tho, acos(cos_tho)*180/3.14159);
				}
			}
			
			//follow inward/outward ness of the previous vector R
			bool ward = Point::anchor_outward_D(SL[i-1].R, P_cur,P_nxt);
			SL[i].bevel_at_positive = ! Point::anchor_outward( SL[i].vP, P_cur,P_nxt, !ward);
				SL[i].vR.follow_signs( SL[i].vP);
			SL[i].T_outward_at_opp = Point::anchor_outward( SL[i].R, P_cur,P_las, !ward);
				SL[i].T.follow_signs( SL[i].R);
			SL[i].T1_outward_at_opp = Point::anchor_outward( SL[i].R1, P_cur,P_nxt, !ward);
				SL[i].T1.follow_signs( SL[i].R1);
			
			if ( d180_degree || zero_degree)
			{	//to solve visual bugs 3 and 1.1
				//efficiency: if color and weight is same as previous and next point
				// ,do not generate vertices
				same_side_of_line( SL[i].R, SL[i-1].R, P_cur,P_las);
					SL[i].T.follow_signs(SL[i].R);
				SL[i].vP=SL[i].T;
				SL[i].vR=SL[i].R;
			}
		} //2nd to 2nd last point
	}
	
	{	int i=2;

		Point bR;
		double r,t;
		make_T_R_C( P[i-1],P[i], &T2,&R2,&bR,weight[i],opt,  0,0);
			same_side_of_line( R2, SL[i-1].R, P[i-1],P[i]);
				T2.follow_signs(R2);
		
		SL[i].djoint=LJ_end;
		SL[i].T=T2;
		SL[i].R=R2;
		SL[i].bR=bR;
		SL[i].t=(float)t;
		SL[i].r=(float)r;
		SL[i].degenT = false;
		SL[i].degenR = false;
	}
	
	polyline_late( P,C,SL,size_of_P,cap_start,cap_end);
}

#endif
