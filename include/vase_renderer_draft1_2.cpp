#ifndef VASE_RENDERER_DRAFT1_2_CPP
#define VASE_RENDERER_DRAFT1_2_CPP

#include "vase_renderer_draft1_2.h"

#ifdef VASE_RENDERER_DEBUG
	#define DEBUG printf
#else
	#define DEBUG //
#endif

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
				double* rr, double* tt, float* dist,
				bool segment=false)
{
	double t=1.0,r=0.0;
	Point DP=P2-P1;
	
	//calculate t,r
	determine_t_r( w,t,r);
	
	if ( opt.feather && !opt.no_feather_at_core)
		r *= opt.feathering;
	else if ( segment)
	{
		//TODO: handle correctly for hori/vert segments in a polyline
		if ( Point::negligible(DP.x)) {
			if ( w>0.0 && w<=1.0) {
				t=0.5; r=0.05;
			}
		} else if ( Point::negligible(DP.y)) {
			if ( w>0.0 && w<=1.0) {
				t=0.5; r=0.05;
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
		//all vP,vR are outward
	
	//for djoint==LJ_bevel
	Point T; //core thickness of a line
	Point R; //fading edge of a line
	Point bR; //out stepping vector, same direction as cap
	Point T1,R1; //alternate vectors, same direction as T21
		//all T,R,T1,R1 are outward
	
	//for djoint==LJ_round
	float t,r;
	
	//for degeneration case
	bool degenT; //core degenerate
	bool degenR; //fade degenerate
	bool pre_full; //draw the preceding segment in full
	Point PT,PR;
	float pt; //parameter at intersection
	bool R_full_degen;
	
	//general info
	char djoint; //determined joint
			// e.g. originally a joint is LJ_miter. but it is smaller than critical angle,
			//   should then set djoint to LJ_bevel
	#define LJ_end   101 //used privately by polyline
};

struct _st_anchor
{
	Vec2  P[3]; //point
	Color C[3]; //color
	double W[3];//weight
	
	_st_polyline SL[3];
	vertex_array_holder vah;
};

static void inner_arc( vertex_array_holder& hold, const Point& P, //P: center
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
			//DEBUG( "steps=%d ",i);
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
	
	//DEBUG( "steps=%d ",int((angle2-angle1)/den*r));

	inner_arc( hold, P, C,C2, dangle,angle1,angle2, r,r2, ignor_ends, apparent_P);
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
#ifdef VASE_RENDERER_DEBUG
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
#endif
}
static void annotate( const Point& P)
{
	Color cc;
	annotate(P,cc);
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
static void printpoint( const Point& P, const char* name)
{
	DEBUG("%s(%.4f,%.4f) ",name,P.x,P.y);
	fflush(stdout);
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
	// never called
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
static void push_quad( vertex_array_holder& core,
		const Point& P0, const Point& P1, const Point& P2, const Point& P3,
		const Color& c0, const Color& c1, const Color& c2, const Color& c3,
		bool trans0=0, bool trans1=0, bool trans2=0, bool trans3=0,
		bool strip=1)
{
	core.push3( P0, P1, P2,
		    c0, c1, c2,
	trans0, trans1, trans2);
	
	if ( strip)
	{	core.push3( P1, P2, P3,
			    c1, c2, c3,
		trans1, trans2, trans3);
	}
	else
	{	core.push3( P0, P2, P3,
			    c0, c2, c3,
		trans0, trans2, trans3);
	}
}

struct _st_knife_cut
{
	Point T1[4]; //retained polygon, also serves as input triangle
	Color C1[4]; //
	
	Point T2[4]; //cut away polygon
	Color C2[4]; //
	
	int T1c, T2c; //count of T1 & T2
		//must be 0,3 or 4
};

static int triangle_knife_cut( const Point& kn1, const Point& kn2, const Point& kn_out, //knife
			       const Color* kC0, const Color* kC1, //color of knife
		_st_knife_cut& ST)//will modify for output
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
		
		if ( (0.0 <= kne1 && kne1 <= 1.0) ||
		     (0.0 <= kne2 && kne2 <= 1.0) )
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
		}
	}
	
	return points_cut_away;
}
static void vah_N_knife_cut( vertex_array_holder& in, vertex_array_holder& out,
		const Point* kn0, const Point* kn1, const Point* kn2,
		const Color* kC0, const Color* kC1,
		int N)
{	//an iterative implementation
	const int MAX_ST = 10;
	_st_knife_cut ST[MAX_ST];
	
	bool kn_colored = kC0 && kC1;
	
	if ( N > MAX_ST)
	{
		DEBUG("vah_N_knife_cut: max N for current build is %d\n", MAX_ST);
		N = MAX_ST;
	}
	
	for ( int i=0; i<in.get_count(); i+=3) //each input triangle
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
static void vah_dual_knife_cut( vertex_array_holder& in, vertex_array_holder& out,
		const Point& kn0, const Point& kn1, const Point& kn2)
{
	_st_knife_cut ST1, ST2a, ST2b;
	for ( int i=0; i<in.get_count(); i+=3)
	{
		//first cut
		ST1.T1[0] = in.get(i);
		ST1.T1[1] = in.get(i+1);
		ST1.T1[2] = in.get(i+2);
			ST1.C1[0] = in.get_color(i);
			ST1.C1[1] = in.get_color(i+1);
			ST1.C1[2] = in.get_color(i+2);
		
		int result1 = triangle_knife_cut( kn0,kn1,kn2,0,0, ST1);
		
		//second cut
		ST2a.T1[0] = ST1.T2[0];
		ST2a.T1[1] = ST1.T2[1];
		ST2a.T1[2] = ST1.T2[2];
			ST2a.C1[0] = ST1.C2[0];
			ST2a.C1[1] = ST1.C2[1];
			ST2a.C1[2] = ST1.C2[2];
		int result2a = triangle_knife_cut( kn2,kn1,kn0,0,0, ST2a);
		
		if ( ST1.T2c > 3)
		{
			ST2b.T1[0] = ST1.T2[1];
			ST2b.T1[1] = ST1.T2[2];
			ST2b.T1[2] = ST1.T2[3];
				ST2b.C1[0] = ST1.C2[1];
				ST2b.C1[1] = ST1.C2[2];
				ST2b.C1[2] = ST1.C2[3];
			int result2b = triangle_knife_cut( kn2,kn1,kn0,0,0, ST2b);
		}
		
		//push results
		if ( ST1.T1c > 0) {
			out.push( ST1.T1[0], ST1.C1[0]);
			out.push( ST1.T1[1], ST1.C1[1]);
			out.push( ST1.T1[2], ST1.C1[2]);
			if ( ST1.T1c > 3) {
				out.push( ST1.T1[1], ST1.C1[1]);
				out.push( ST1.T1[2], ST1.C1[2]);
				out.push( ST1.T1[3], ST1.C1[3]);
			}
		}
		
		if ( ST2a.T1c > 0) {
			out.push( ST2a.T1[0], ST2a.C1[0]);
			out.push( ST2a.T1[1], ST2a.C1[1]);
			out.push( ST2a.T1[2], ST2a.C1[2]);
			if ( ST2a.T1c > 3) {
				out.push( ST2a.T1[1], ST2a.C1[1]);
				out.push( ST2a.T1[2], ST2a.C1[2]);
				out.push( ST2a.T1[3], ST2a.C1[3]);
			}
		}
		
		if ( ST1.T2c > 3) {
			if ( ST2b.T1c > 0) {
				out.push( ST2b.T1[0], ST2b.C1[0]);
				out.push( ST2b.T1[1], ST2b.C1[1]);
				out.push( ST2b.T1[2], ST2b.C1[2]);
				if ( ST2b.T1c > 3) {
					out.push( ST2b.T1[1], ST2b.C1[1]);
					out.push( ST2b.T1[2], ST2b.C1[2]);
					out.push( ST2b.T1[3], ST2b.C1[3]);
				}
			}
		}
		
		/* //triangle strip
		for ( int j=0; j<ST2a.T1c; j++)
			out.push( ST2a.T1[j], ST2a.C1[j])*/
	}
}
static void vah_knife_cut( vertex_array_holder& core, //serves as both input and output
		const Point& kn1, const Point& kn2, const Point& kn_out)
{
	_st_knife_cut ST;
	for ( int i=0; i<core.get_count(); i+=3)
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

#if 0
void polyline_late( const Vec2* P, const Color* C, _st_polyline* SL, int size_of_P, Point cap1, Point cap2)
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
					dangle, SL[i].t, 0.0f, false, &P10, false);
					core.push( P2,C[i]);
					
					//Point apparent_P = P_a*1.0 + P_b*0.0 + P_cur;
					//Point apparent_P = P7; //may have caused a tiny artifact
				}
				else
				{
					//circular fan by degenerated triangles
					vectors_to_arc( core, P_cur, C[i], C[i],
					P7-P_cur, P2-P_cur,
					dangle, SL[i].t, 0.0f, false, &P10, false);
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
			//debug only //if ( i==0) draw_vector(P_cur,SL[i].T,"T");
			
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
					dangle, SL[i].t, 0.0f, true, &apparent_P, false);

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
				
				Color cbt = Color_between(C[i],C[i+1],
				(SL[i].length*0.5f) / SL[i+1].length); //must sync with TAG-01
				
				switch (SL[i].djoint)
				{
				case LJ_miter:
					if ( K != Q) {
						goto fix_incomplete_welding_bevel_round;
					} else {
						core.push( plus_minus(P_cur,SL[i].vP, K), C[i]);
						core.push( SL[i].PT, cbt);
					}
				break;
				case LJ_bevel:
				case LJ_round:
				fix_incomplete_welding_bevel_round:
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
					Point P2 = plus_minus(P_cur,SL[i].vP, !Q);
					Point P3 = SL[i].PT;
					Point P4 = P_las - plus_minus(SL[i-1].T, !SL[i-1].T_outward_at_opp);
					Point cen= (P3+P4)*0.5;
					
					Color cbt = Color_between(C[i],C[i+1],
					(SL[i].length*0.5f) / SL[i+1].length); //must sync with TAG-01
					
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
						dangle, SL[i].t, SL[i].t+SL[i].r, false, 0, false);
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
#endif

const float cri_core_adapt = 0.0001f;
static void anchor_late( const Vec2* P, const Color* C, _st_polyline* SL,
		vertex_array_holder& tris,
		Point cap1, Point cap2)
{	const int size_of_P = 3;

	tris.set_gl_draw_mode(GL_TRIANGLES);
	
	Point P_0, P_1, P_2;
	P_0 = Point(P[0])-cap1;
	P_1 = Point(P[1]);
	P_2 = Point(P[2])-cap2;
	
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
		{	//normal last segment
			tris.push3( P1,  P5,  P6,
				  C[1],C[1],C[2]);
			tris.push3( P1,  P6,  P7,
				  C[1],C[2],C[2]);

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
		{	//normal first segment
			tris.push3( P3,  P2,  P1,
				  C[0],C[1],C[1]);
			tris.push3( P1,  P3,  P4,
				  C[1],C[0],C[0]);

			//special last segment
			Point P9 = SL[1].PT;
			tris.push3( P1,  P5,  P6,
				  C[1],C[1],C[2]);
			tris.push3( P1,  P9,  P6,
				  C[1], Cpt,C[2]);
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
	{	//normal line core
		//first segment
		tris.push3( P3,  P2,  P1,
			  C[0],C[1],C[1]);
		tris.push3( P1,  P3,  P4,
			  C[1],C[0],C[0]);
		//last segment
		tris.push3( P1,  P5,  P6,
			  C[1],C[1],C[2]);
		tris.push3( P1,  P6,  P7,
			  C[1],C[2],C[2]);
	}
	
	if ( SL[1].degenR)
	{	//degen inner fade
		Point P9 = SL[1].PT;
			Point P9r = SL[1].PR;
		
		Color ccpt=Cpt;
		if ( SL[1].degenT)
			ccpt = C[1];
		
		if ( SL[1].pre_full)
		{	push_quad( tris,
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
		{	push_quad( tris,
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
		push_quad( tris,
			    P1,  P4, P1r, P4r,
			  C[1],C[0],C[1],C[0],
			     0,   0,   1,   1); //fir seg
		push_quad( tris,
			    P1,  P7, P1r, P7r,
			  C[1],C[2],C[1],C[2],
			     0,   0,   1,   1); //las seg
	}
	
	switch( SL[1].djoint)
	{	//line core joint
		case LJ_miter:
			tris.push3( P2,  P5,  P0,
				  C[1],C[1],C[1]);
		case LJ_bevel:
			tris.push3( P2,  P5,  P1,
				  C[1],C[1],C[1]);
		break;
		
		case LJ_round: {
			vertex_array_holder strip;
			strip.set_gl_draw_mode(GL_TRIANGLE_STRIP);
			
			vectors_to_arc( strip, P_1, C[1], C[1],
			SL[1].T1, SL[1].T,
			get_LJ_round_dangle(SL[1].t,SL[1].r),
			SL[1].t, 0.0f, false, &P1);
			
			tris.push(strip);
		} break;
	}
	
	{	//outer fade, whether degen or normal
		push_quad( tris,
			    P2,  P3, P2r, P3r,
			  C[1],C[0],C[1],C[0],
			     0,   0,   1,   1); //fir seg
		push_quad( tris,
			    P5,  P6, P5r, P6r,
			  C[1],C[2],C[1],C[2],
			     0,   0,   1,   1); //las seg
		switch( SL[1].djoint)
		{	//line fade joint
			case LJ_miter:
				push_quad( tris,
					    P0,  P5, P0r, P5r,
					  C[1],C[1],C[1],C[1],
					     0,   0,   1,   1);
				push_quad( tris,
					    P0,  P2, P0r, P2r,
					  C[1],C[1],C[1],C[1],
					     0,   0,   1,   1);
			break;
			case LJ_bevel:
				push_quad( tris,
					    P2,  P5, P2r, P5r,
					  C[1],C[1],C[1],C[1],
					     0,   0,   1,   1);
			break;
			
			case LJ_round: {
				vertex_array_holder strip;
				strip.set_gl_draw_mode(GL_TRIANGLE_STRIP);
				Color C2 = C[1]; C2.a = 0.0f;
				vectors_to_arc( strip, P_1, C[1], C2,
				SL[1].T1, SL[1].T,
				get_LJ_round_dangle(SL[1].t,SL[1].r),
				SL[1].t, SL[1].t+SL[1].r, false, 0);
				
				tris.push(strip);
			} break;
		}
	}
	
	{	//caps
	vertex_array_holder cap[2];
	cap[0].set_gl_draw_mode(GL_TRIANGLES);
	cap[1].set_gl_draw_mode(GL_TRIANGLES);
	for ( int i=0,k=0; k<=1; i=size_of_P-1, k++)
	{
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
		
		Point& cur_cap = i==0? cap1:cap2;
		if ( cur_cap.non_zero()) //save some degenerated points
		{
			Point P0,P1,P2,P3,P4,P5,P6;
			
			P0 = P_cur+SL[i].T+SL[i].R;
			P1 = P0+cur_cap;
			P2 = P_cur+SL[i].T;
			P4 = P_cur-SL[i].T;
			P3 = P4-SL[i].R+cur_cap;
			P5 = P4-SL[i].R;
			
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
			tris.push(cap[k]);
		}
	}
	}
}

static void segment_late( const Vec2* P, const Color* C, _st_polyline* SL,
		vertex_array_holder& tris,
		Point cap1, Point cap2)
{
	tris.set_gl_draw_mode(GL_TRIANGLES);
	
	Point P_0, P_1, P_2;
	P_0 = Point(P[0])-cap1;
	P_1 = Point(P[1])-cap2;
	
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
	push_quad( tris,
		  P1, P1r,  P3, P3r,
		C[0],C[0],C[1],C[1],
		   0,   1,   0,   1 );
	push_quad( tris,
		  P2, P2r,  P4, P4r,
		C[0],C[0],C[1],C[1],
		   0,   1,   0,   1 );
	//caps
	for ( int j=0; j<2; j++)
	{
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
		
		tris.push( Pkr, C[j], 1);
		tris.push( Pkc, C[j], 1);
		tris.push( Pk , C[j], 0);
				tris.push( Pkc, C[j], 1);
			tris.push( Pk , C[j], 0);
		tris.push( Pjc, C[j], 1);
		
				tris.push( Pk , C[j], 0);
			tris.push( Pjc, C[j], 1);
		tris.push( Pj , C[j], 0);
		
				tris.push( Pjc, C[j], 1);
			tris.push( Pj , C[j], 0);
		tris.push( Pjr, C[j], 1);
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

static void segment( const Vec2* P, const Color* C, const double* weight, polyline_opt* options, 
		bool cap_first, bool cap_last)
{
	if ( !P || !C || !weight) return;
	
	polyline_opt opt={0};
	if ( options)
		opt = (*options);
	
	Point T1,T2,T21,T31;		//]these are for calculations in early stage
	Point R1,R2,R21,R31;		//]
	
	Point cap_start, cap_end;
	_st_polyline SL[2];
	
	Color col_buf[2];
	if ( (weight[0]>=0.0 && weight[0]<1.0) ||
		(weight[1]>=0.0 && weight[1]<1.0) )
	{	//we should replace the original color buffer with a new one
		for ( int i=0; i<2; i++)
		{
			col_buf[i] = C[i];
			if ( weight[i]>=0.0 && weight[i]<1.0)
			{
				double f=weight[i]-static_cast<int>(weight[i]);
				col_buf[i].a *=f;
			}
		}
		C = col_buf;
	}
	
	{	int i=0;
	
		Point cap1;
		make_T_R_C( Point(P[i]), Point(P[i+1]), &T2,&R2,&cap1, weight[i],opt, 0,0,0, true);
		
		if ( cap_first)
		{
			cap_start = cap1;
			cap_start.opposite(); if ( opt.feather && !opt.no_feather_at_cap)
			cap_start*=opt.feathering;
		}
		
		SL[i].djoint=LJ_end;
		SL[i].T=T2;
		SL[i].R=R2;
		SL[i].bR=cap1;
		SL[i].degenT = false;
		SL[i].degenR = false;
	}
	
	{	int i=1;

		Point bR;
		make_T_R_C( P[i-1],P[i], &T2,&R2,&bR,weight[i],opt,  0,0,0, true);
		
		if ( cap_last)
		{
			cap_end = bR;
			if ( opt.feather && !opt.no_feather_at_cap)
				cap_end*=opt.feathering;
		}
		
		SL[i].djoint=LJ_end;
		SL[i].T=T2;
		SL[i].R=R2;
		SL[i].bR=bR;
		SL[i].degenT = false;
		SL[i].degenR = false;
	}
	
	{	vertex_array_holder tris;
		
		segment_late( P,C,SL, tris,cap_start,cap_end);
		
		tris.draw();
	}
}

static void anchor( _st_anchor& SA, polyline_opt* options,
		bool cap_first, bool cap_last)
{
	Vec2*  P = SA.P;
	Color* C = SA.C;
	double* weight = SA.W;
	
	polyline_opt opt={0};
	if ( options)
		opt = (*options);
	
	_st_polyline* SL = SA.SL;
	SA.vah.set_gl_draw_mode(GL_TRIANGLES);
	
	//const double critical_angle=11.6538;
	/*critical angle in degrees where a miter is force into bevel
	 * it is _similar_ to cairo_set_miter_limit ()
	 * but cairo works with ratio while we work with included angle*/
	const double cos_cri_angle=0.979386; //cos(critical_angle)
	
	Point T1,T2,T21,T31;		//]these are for calculations in early stage
	Point R1,R2,R21,R31;		//]
	
	Point cap_start, cap_end;	//for anchor_late
	
	bool varying_weight = !(weight[0]==weight[1] & weight[1]==weight[2]);
	
	Color col_buf[3];
	if ( (weight[0]>=0.0 && weight[0]<1.0) ||
		(weight[1]>=0.0 && weight[1]<1.0) ||
		(weight[2]>=0.0 && weight[2]<1.0) )
	{	//we should replace the original color buffer with a new one
		for ( int i=0; i<3; i++)
		{
			col_buf[i] = C[i];
			if ( weight[i]>=0.0 && weight[i]<1.0)
			{
				double f=weight[i]-static_cast<int>(weight[i]);
				col_buf[i].a *=f;
			}
		}
		C = col_buf;
	}
	
	{	int i=0;
	
		Point cap1;
		double r,t;
		make_T_R_C( Point(P[i]), Point(P[i+1]), &T2,&R2,&cap1, weight[i],opt, &r,&t,0);
		if ( varying_weight) {
		make_T_R_C( Point(P[i]), Point(P[i+1]), &T31,&R31,0, weight[i+1],opt, 0,0,0);
		} else {
			T31 = T2;
			R31 = R2;
		}
		Point::anchor_outward(R2, P[i+1],P[i+2] /*,inward_first->value()*/);
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
		make_T_R_C( P[i-1],P[i], 0,0,&cap2,weight[i],opt, 0,0,0);
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
			
			if ( cos_tho < 0 || opt.joint==LJ_round)
			{	//when greater than 90 degrees
				SL[i-1].bR = Point();
				SL[i]  .bR = Point();
				SL[i+1].bR = Point();
				//to solve an overdraw in bevel joint
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
						PT1, 0,&pt1); //core
			int result2t = Point::intersect( P_las-T1, P_las+T1,
						P_nxt+T31, P_cur+T2, //knife2_a
						PT2, 0,&pt2);
			bool is_result1t = result1t == 1;
			bool is_result2t = result2t == 1;
			//
			if ( zero_degree)
			{
				bool pre_full = is_result1t;
				if ( pre_full)
				{
					segment( P,C,weight,options,cap_first,cap_last);
				}
				else
				{
					segment( &P[1],&C[1],&weight[1],options,cap_first,cap_last);
				}
				return;
			}
			
			if ( is_result1r | is_result2r)
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
			
			if ( is_result1t | is_result2t)
			{	//core degeneration
				SL[i].degenT=true;
				SL[i].pre_full=is_result1t;
				SL[i].PT = is_result1t? PT1:PT2;
				SL[i].pt = float(is_result1t? pt1:pt2);
			}
			
			//make joint
			SL[i].djoint = opt.joint;
			if ( opt.joint == LJ_miter)
				if ( cos_tho >= cos_cri_angle)
					SL[i].djoint=LJ_bevel;
			
			/*if ( varying_weight && smaller_than_30_degree)
			{	//not sure why, but it appears to solve a visual bug for varing weight
				Point interR,vR;
				int result3 = Point::intersect( P_las-T1-R1, P_cur-T21-R21,
							P_nxt-T31-R31, P_cur-T2-R2,
							interR);
				SL[i].vR = P_cur-interR-SL[i].vP;
				annotate(interR,C[i],9);
				draw_vector(P_las-T1-R1, P_cur-T21-R21 - P_las+T1+R1,"1");
				draw_vector(P_nxt-T31-R31, P_cur-T2-R2 - P_nxt+T31+R31,"2");
			}*/
			
			//follow inward/outward ness of the previous vector R
			bool ward = Point::anchor_outward_D(SL[i-1].R, P_cur,P_nxt);
			/*SL[i].bevel_at_positive = !*/ Point::anchor_outward( SL[i].vP, P_cur,P_nxt, !ward);
				SL[i].vR.follow_signs( SL[i].vP);
			Point::anchor_outward( SL[i].R, P_cur,P_las, !ward);
				SL[i].T.follow_signs( SL[i].R);
			Point::anchor_outward( SL[i].R1, P_cur,P_nxt, !ward);
				SL[i].T1.follow_signs( SL[i].R1);
			
			if ( d180_degree)
			{	//to solve visual bugs 3 and 1.1
				//efficiency: if color and weight is same as previous and next point
				// ,do not generate vertices
				same_side_of_line( SL[i].R, SL[i-1].R, P_cur,P_las);
					SL[i].T.follow_signs(SL[i].R);
				SL[i].vP=SL[i].T;
				SL[i].vR=SL[i].R;
				SL[i].djoint=LJ_miter;
			}
		} //2nd to 2nd last point
	}
	
	{	int i=2;

		Point bR;
		double r,t;
		make_T_R_C( P[i-1],P[i], &T2,&R2,&bR,weight[i],opt,  0,0,0);
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

	anchor_late( P,C, SL,SA.vah, cap_start,cap_end);
}

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

void polyline(
	const Vec2* P,       //pointer to array of point of a polyline
	const Color* C,      //array of color
	const double* weight,//array of weight
	int size_of_P, //size of the buffer P
	polyline_opt* options) //extra options
{
	Vec2  PP[3];
	Color CC[3];
	double WW[3];
	
	Point mid_l, mid_n; //the last and the next mid point
	Color c_l, c_n;
	double w_l, w_n;
	{	//init for the first anchor
		mid_l = P[0];
		c_l = C[0];
		w_l = weight[0];
	}
	
	int k=0; //number of anchors
	
	if ( size_of_P == 2)
	{
		segment( P,C,weight,options, true,true);
		return;
	}
	
	circular_array<_st_anchor> C_A(2);
	for ( int j=0; j<C_A.get_size(); j++)
	{
		C_A[j].SL[1].degenR=false;
	}
	
	for ( int i=1; i<size_of_P-1; i++)
	{
		_st_anchor& AC_cur = C_A[0];
		_st_anchor& AC_las = C_A[-1];
		
		if ( i == size_of_P-2) {
			mid_n = P[i+1];
			c_n   = C[i+1];
			w_n   = weight[i+1];
		}
		else {
			mid_n = Point::midpoint(P[i],P[i+1]);
			c_n   = Color_between  (C[i],C[i+1]);
			w_n   = (weight[i]+weight[i+1]) *0.5;
		}
		
		AC_cur.P[0]=mid_l.vec(); AC_cur.C[0]=c_l;  AC_cur.W[0]=w_l;
		AC_cur.P[1]=P[i];        AC_cur.C[1]=C[i]; AC_cur.W[1]=weight[i];
		AC_cur.P[2]=mid_n.vec(); AC_cur.C[2]=c_n;  AC_cur.W[2]=w_n;
		
		k++; anchor( AC_cur,options, i==1,i==size_of_P-2);
		
		{	//handle overlapping between anchors
			bool degen_las = AC_las.SL[1].degenR && AC_las.SL[1].pre_full;
			bool degen_cur = AC_cur.SL[1].degenR && ! AC_cur.SL[1].pre_full;
			
			Point P1,P2,P3,P4;
			Color C1,C2,C3,C4;
			vertex_array_holder* target = 0;
			
			if ( degen_las)
			{
				_st_polyline* SL = AC_las.SL;
				Point P_0 = AC_las.P[0];
				Point P_1 = AC_las.P[1];
				
				P1 = P_0+SL[0].T+SL[0].R;
				P2 = P_0-SL[0].T;
				P3 = P_1+SL[1].T1+SL[1].R1;
				P4 = P_1+SL[1].T; //P4 = SL[1].PT;
					C1 = AC_las.C[0];
					C2 = AC_las.C[0];
					C3 = AC_las.C[1];
					C4 = AC_las.C[1];
					
				//~ 1--2 [P_0]
				//~ |  |
				//~ |  |
				//~ 3  4 [P_1]
				
				target = &AC_cur.vah;
			}
			
			if ( target)
			{
					     // kn|kn|...    (view each knife vertically)
				Point kn0[] = { P1,P2,P3,P4};
				Point kn1[] = { P2,P4,P1,P3};
				Point kn2[] = { P4,P3,P2,P1};
					Color kC0[] = { C1,C2,C3,C4};
					Color kC1[] = { C2,C4,C1,C3};
				
				vertex_array_holder vah_out;
				vah_out.set_gl_draw_mode(GL_TRIANGLES);
				
				vah_N_knife_cut( *target, vah_out,
						kn0,kn1,kn2,
						kC0,kC1,
						4);
				
				vah_out.draw();
				target->clear();
			}
		}
		
		mid_l = mid_n;
		c_l = c_n;
		w_l = w_n;
		
		//AC_las.vah.draw();
		//AC_las.vah.clear();
		//if ( i==size_of_P-2)
		{
			AC_cur.vah.draw();
			AC_cur.vah.clear();
		}
		
		C_A.move(1);
	}
}

#undef DEBUG
#endif
