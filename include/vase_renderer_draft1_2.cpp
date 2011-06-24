#ifndef VASE_RENDERER_DRAFT1_2_CPP
#define VASE_RENDERER_DRAFT1_2_CPP

#include "vase_renderer_draft1_2.h"

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
	Point p_out;
	bool curve_form_gamma;
	
	char djoint; //determined joint
			// e.g. originally a joint is LJ_miter. but it is smaller than critical angle,
			//   should then set djoint to LJ_bevel
	#define LJ_end   101 //used privately by polyline
	
};
class st_polyline
{
	_st_polyline* SL;
	int N;
public:
	st_polyline( int size_of_P)
	{
		N = size_of_P;
		SL= new _st_polyline[N];
	}
	~st_polyline()
	{
		delete[] SL;
	}
	
	_st_polyline& operator[](int I) const
	{
		if ( I<0){
			I=0;
			printf("st_polyline: memory error! I<0\n");
		}
		if ( I>N-1){
			I=N-1;
			printf("st_polyline: memory error! I>N-1\n");
		}
		return SL[I];
	}
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
	Point P2 = P+V;
	glBegin(GL_LINES);
		glColor3f(1,0,0);
		glVertex2f(P.x,P.y);
		glColor3f(1,0.9,0.9);
		glVertex2f(P2.x,P2.y);
	glEnd();
	if ( name)
	{
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

static void polyline_late( Vec2* P, Color* C, const st_polyline& SL, int size_of_P, Point cap1, Point cap2)
{
	vertex_array_holder core;
	vertex_array_holder fade[2];
	vertex_array_holder cap[2];
	
	core.set_gl_draw_mode(GL_TRIANGLE_STRIP);
	fade[0].set_gl_draw_mode(GL_TRIANGLE_STRIP);
	fade[1].set_gl_draw_mode(GL_TRIANGLE_STRIP);
	cap[0].set_gl_draw_mode(GL_TRIANGLE_STRIP);
	cap[1].set_gl_draw_mode(GL_TRIANGLE_STRIP);
	
	//memory optimization for CPU
	//  the loops here can be performed inside the big loop in polyline()
	//  thus saving the memory used in (new _st_polyline[size_of_P])
	
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
			if ( SL[i].pre_full)
				if ( i < size_of_P-1)
					if ( SL[i+1].djoint != LJ_end)
						P10 = SL[i].p_out;
			
			switch( SL[i].djoint)
			{
			case LJ_miter:
			{
				Point interP = plus_minus( P_cur,SL[i].vP, Q);
				if ( K != Q)
				{
					core.push( P10, C[i+1]);
					core.push( interP, C[i]);
				}
				else
				{
					core.push( interP, C[i]);
					core.push( P10, C[i+1]);
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
					core.push( P10,C[i+1]);
					core.push( P7,C[i]);
					core.push( P10,C[i+1]);
					core.push( P2,C[i]);
				}
				else
				{
					core.push( P7,C[i]);
					core.push( P10,C[i+1]);
					core.push( P2,C[i]);
					core.push( P10,C[i+1]);
					K = !K;
				}
			} break;
			
			case LJ_round:
			{
				Point P2, P7;
				P2 = plus_minus(P_cur,SL[i].T, Q);
				P7 = plus_minus(P_cur,SL[i].T1, Q);
				
				float dangle = get_LJ_round_dangle(SL[i].t,SL[i].r);
				if ( K != Q)
				{
					core.push( P10,C[i+1]);
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
				if ( !K)
				{	//10 6
					core.push(SL[ia].PT,C[i]);
					core.push(P6,C[i]);
				}
				else
				{
					core.push(P6,C[i]);
					core.push(SL[ia].PT,C[i]);
				}
			break;
			
			case LJ_bevel:
			case LJ_miter: 
			{
				Point& P10 = SL[ia].p_out;
				
				if ( !SL[ia].curve_form_gamma)
				{
					Point P1 = plus_minus(P_cur,SL[i].vP,!Q);
					if ( !K)
					{
						core.push(P10,C[i]);
						core.push(P1,C[i]);
					}
					else
					{
						core.push(P1,C[i]);
						core.push(P10,C[i]);
					}
				}
				else
				{
					Point P1a= plus_minus(P_cur,SL[i].vP, Q);
					if ( !K)
						core.repeat_last_push();
					
					core.push(P1a,C[i]);
					core.push(P10,C[i]);
					if ( SL[ia].djoint!=LJ_miter) {
						K = !K;
					} else {
						if ( !K) {
							K = !K;
						}
					}
				}
			} break;
			
			case LJ_round:
			break;
			}
		}
		else
		{
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
		}
		
		//fade
		/*for ( int E=0; E<=1; E++)
		{
			bool degen_nxtR = 0;
			bool degen_lasR = 0;
			if ( i < size_of_P-2)
				degen_nxtR = SL[i+1].degenR && ! SL[i+1].pre_full;
			if ( i > 1)
				degen_lasR = SL[i-1].degenR && SL[i-1].pre_full;
			
			if ( SL[i].degenR)
			{
				if ( Q != bool(E))
				{
					if ( SL[i].degenT)
					{	// SL[i].degenR && SL[i].degenT
					
						fade[E].push(SL[i].PT,C[i]);
						fade[E].push(SL[i].PR,C[i],true);
						continue; //skip normal fade
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
						
						continue; //skip normal fade
					}
				}
			}
			else if ( degen_nxtR)
			{
				if ( SL[i+1].bevel_at_positive != bool(E))
				{
					Point P3 = plus_minus(P_cur,SL[i].T, E);
					Point P0 = SL[i+1].PR;
					fade[E].push(P3,C[i]);
					fade[E].push(P0,C[i],true);
					continue;
				}
			}
			else if ( degen_lasR)
			{
				if ( SL[i-1].bevel_at_positive != bool(E))
				{
				}
			}
			
			switch (SL[i].djoint)
			{	//normal fade
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
					fade[E].push( plus_minus(P_cur,SL[i].T1+SL[i].R1-SL[i].bR, E), C[i],true);
					fade[E].push( plus_minus(P_cur,SL[i].T, E), C[i]);
					fade[E].push( plus_minus(P_cur,SL[i].T+SL[i].R-SL[i+1].bR, E), C[i],true);
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
		if ( cur_cap.non_zero()) //I don't want degeneration
		{
			cap[k].push( P_cur+SL[i].T+SL[i].R, C[i],true);
			cap[k].push( P_cur+SL[i].T+SL[i].R+cur_cap, C[i],true);
			cap[k].push( P_cur+SL[i].T, C[i]);
			cap[k].push( P_cur-SL[i].T-SL[i].R+cur_cap, C[i],true);
			cap[k].push( P_cur-SL[i].T, C[i]);
			cap[k].push( P_cur-SL[i].T-SL[i].R, C[i],true);
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
}

static void polyline_last_degen( st_polyline& SL, int i, Vec2* P, const Point& T2)
{
	int i_1, if_1 , i_2;
	i_1 = i-1;
	i_2 = i-2;
	if_1= i+1;
	
	Point llas_T = plus_minus( SL[i_2].T, !SL[i_1].T_outward_at_opp);
	Point P7,P2, P8,P9, P81,P91;
	P7 = Point(P[i]) + SL[i].vP -SL[i].bR*5;
	P2 = Point(P[if_1])+T2;
	P8 = Point(P[i_1])-llas_T;
	P9 = Point(P[i_2])-llas_T;
	P81= Point(P[i_1])+llas_T -SL[i].bR*5;
	P91= Point(P[i_2])+llas_T -SL[i].bR*5;
	//involve SL[i].bR*5 in P7,P81,P91 is to solve a visual bug
	//  which is caused by an attempt to solve another bug
	
	if ( Point::signed_area(P7,P8,P9) > 0 !=
		Point::signed_area(P7,P81,P91) > 0)
	{
		int result4 = Point::intersect( P7, P2,
				P8, P9,
				SL[i_1].p_out);
	}
	else
	{
		Point P6,P21;
		P6 = Point(P[i])-T2;
		P21= Point(P[if_1])-T2;
		
		SL[i_1].curve_form_gamma = true;
		int result5 = Point::intersect( P6, P21,
				P8, P9,
				SL[i_1].p_out);
	}
	
	Color C;
	annotate(P2,C,2);
	annotate(P7,C,7);
	annotate(P8,C,8);
	annotate(P9,C,9);
	annotate(P81,C,81);
	annotate(P91,C,91);
	annotate(SL[i_1].p_out, C,10);
}

static void polyline_next_degen( st_polyline& SL, int i, Vec2* P, const Point& T2)
{
	int i_1, if_1 , i_2;
	i_1 = i+1;
	i_2 = i+2;
	if_1= i-1;
	
	Point llas_T = plus_minus( SL[i_2].T, !SL[i_1].T_outward_at_opp);
	Point P7,P2, P8,P9, P81,P91;
	Color C;
	
	P7 = Point(P[i]) - SL[i].vP -SL[i].bR*5;
	annotate(P7,C, 7);
}

void polyline( Vec2* P, Color* C, double* weight, int size_of_P, polyline_opt* options)
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
	st_polyline SL(size_of_P);			//[to be passed to polyline_late()
	
	//early stage
	
	{	int i=0;
		Point cap1;
		make_T_R_C( Point(P[i]), Point(P[i+1]), 0,0,&cap1, weight[i],opt, 0,0);
		
		cap1.opposite(); if ( opt.feather && !opt.no_feather_at_cap)
		cap1*=opt.feathering;
		cap_start = cap1;
	}
	{	int i=size_of_P-1;
		Point cap2;
		make_T_R_C( P[i-1],P[i], 0,0,&cap2,weight[i],opt, 0,0);
		
		if ( opt.feather && !opt.no_feather_at_cap)
			cap2*=opt.feathering;
		cap_end = cap2;
	}
	
	for ( int i=0; i<size_of_P; i++)
	{
		double r,t;
		
		SL[i].degenT = false;
		SL[i].degenR = false;
		SL[i].curve_form_gamma = false;
		
		//TODO: handle correctly when there are only 2 points
		if ( weight[i]>=0.0 && weight[i]<1.0)
		{
			double f=weight[i]-static_cast<int>(weight[i]);
			C[i].a *=f;
		}
		if ( i < size_of_P-1)
		{
			Point bR;  Point C31;
			make_T_R_C( Point(P[i]), Point(P[i+1]),  &T2,&R2,&bR, weight[i],opt, &r,&t);			
			make_T_R_C( Point(P[i]), Point(P[i+1]), &T31,&R31,&C31, weight[i+1],opt, 0,0);
			
			//TODO: (optional) weight compensation when segment is < 1.0px and exactly hori/ vertical
			
			if ( i==0) {
				Point::anchor_outward(R2, P[i+1],P[i+2]);
					T2.follow_signs(R2);
				SL[i].djoint=LJ_end;
			}
			SL[i].T=T2;
			SL[i].R=R2;
			SL[i].bR=bR;
			SL[i].t=(float)t;
			SL[i].r=(float)r;
			
			SL[i+1].T1=T31;
			SL[i+1].R1=R31;
		}
		
		if ( i==0) {
			//do nothing
		} else if ( i==size_of_P-1) {
			Point bR;
			make_T_R_C( P[i-1],P[i],   &T2,&R2,&bR,weight[i],opt,  0,0);
				same_side_of_line( R2, SL[i-1].R, P[i-1],P[i]);
					T2.follow_signs(R2);
			
			SL[i].T=T2;
			SL[i].R=R2;
			SL[i].bR=bR;
			SL[i].djoint=LJ_end;
		}
		else //2nd to 2nd last point
		{
			Point P_cur = P[i]; //current point //to avoid calling constructor repeatedly
			Point P_nxt = P[i+1]; //next point
				if ( i == size_of_P-2)
					P_nxt -= cap_end;
			Point P_las = P[i-1]; //last point
				if ( i == 1)
					P_las -= cap_start;
			
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
				SL[i].PR = is_result1r? PR1:PR2;
				SL[i].PT = is_result1t? PT1:PT2;
				SL[i].pre_full = is_result1r;
			}
			
			if ( (is_result1t | is_result2t) && !zero_degree)
			{	//core degeneration
				SL[i].degenT=true;
				SL[i].pre_full=is_result1t;
				//SL[i].PT = is_result1t? PT1:PT2;
				
				if ( !SL[i].pre_full)
				{
					polyline_next_degen(SL,i-1,P,-T2);
				}
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
			
			if ( SL[i-1].degenT && SL[i-1].pre_full)
			{
				polyline_last_degen(SL,i,P,T2);
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
			
		}
		T1=T2;   R1=R2;
		T21=T31; R21=R31;
	} //end of for loop
	
	polyline_late( P,C,SL,size_of_P,cap_start,cap_end);
}

#endif
