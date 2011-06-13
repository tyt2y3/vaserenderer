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

/*known visual bugs:
 * 1.  [solved]when 2 segments are exactly at 90 degrees, the succeeding line segment is reflexed.
 * 1.1 [solved]when 2 segments are exactly at 180 degrees,,
 * 2.  [solved]when polyline is dragged, caps seem to have pseudo-podia.
 * 3.  [solved]when 2 segments are exactly horizontal/ vertical, both segments are reflexed.
 * 3.1 [solved]when a segment is exactly horizontal/ vertical, the cap disappears
 * 4.  when 2 segments make < 5 degrees,,
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
	Point T; //core thickness of a line
	Point R; //fading edge of a line
	Point vP; //vector to intersection point
	Point vR; //fading vector at sharp end
	
	//for djoint==LJ_bevel
	Point bR; //out stepping vector, same direction as cap
	Point T1,R1; //alternate vectors, same direction as T21
	
	//for djoint==LJ_round
	float t,r;
	
	//all vectors except bR
	//  must point to the same side of the polyline
	
	char djoint; //determined joint
			// e.g. originally a joint is LJ_miter. but it is smaller than critical angle,
			//   should then set djoint to LJ_bevel
	#define LJ_end 101//used privately by polyline
	
	bool bevel_at_positive; //used only when djoint==LJ_bevel
};

void inner_arc_strip( vertex_array_holder& hold, const Point& P, //P: center
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
					printf("trapped in loop: inc,ig_end"
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
					printf("trapped in loop: inc,end"
						"angle1=%.2f, angle2=%.2f, dangle=%.2f\n",
						angle1, angle2, dangle);
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
				hold.push( Point(P.x+x*r,P.y-y*r), C);
				if ( !apparent_P)
					hold.push( Point(P.x+x*r2,P.y-y*r2), C2);
				else
					hold.push( *apparent_P, C2);
				
				if ( i>100) {
					printf("trapped in loop: dec,ig_end"
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
					printf("trapped in loop: dec,end"
						"angle1=%.2f, angle2=%.2f, dangle=%.2f\n",
						angle1, angle2, dangle);
					break;
				}
			}
		}
	}
}
void vectors_to_arc( vertex_array_holder& hold, const Point& P,
		const Color& C, const Color& C2,
		Point A, Point B, float den, float r, float r2, bool ignor_ends,
		Point* apparent_P)
{
	const double& m_pi = vaserend_pi;
	A *= 1/r;
	B *= 1/r;
	float angle1 = acos(A.x);
	float angle2 = acos(B.x);
	if ( A.y>0){ angle1=2*m_pi-angle1;}
	if ( B.y>0){ angle2=2*m_pi-angle2;}

	inner_arc_strip( hold, P, C,C2, den/r,angle1,angle2, r,r2, ignor_ends, apparent_P);
}

static void polyline_late( Vec2* P, Color* C, _st_polyline* SL, int size_of_P, Point cap1, Point cap2)
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
	{	//line core
		switch (SL[i].djoint)
		{
			case LJ_end:
			{
				const Point& cur_cap = i==0? cap1:cap2;
				core.push( plus_minus(P[i],SL[i].T, K)-cur_cap,  C[i]);
				core.push( plus_minus(P[i],SL[i].T, !K)-cur_cap, C[i]);
			} break;
			
			case LJ_miter:
				core.push( plus_minus(P[i],SL[i].vP, K),  C[i]);
				core.push( plus_minus(P[i],SL[i].vP, !K), C[i]);
			break;
			
			case LJ_bevel:
			{
				bool Q = SL[i].bevel_at_positive;
				if (Q) {
					core.push( plus_minus(P[i],SL[i].vP, !Q), C[i]);
					core.push( plus_minus(P[i],SL[i].T1,  Q), C[i]);
					core.push( plus_minus(P[i],SL[i].vP, !Q), C[i]);//sorry for degenerated triangle
					core.push( plus_minus(P[i],SL[i].T,   Q), C[i]);
				} else {
					core.push( plus_minus(P[i],SL[i].T1,  Q), C[i]);
					core.push( plus_minus(P[i],SL[i].vP, !Q), C[i]);
					core.push( plus_minus(P[i],SL[i].T,   Q), C[i]);
					K = !K;
				}
			} break;
			
			case LJ_round:
			{
				bool Q = SL[i].bevel_at_positive;
				//preceding points
				if (Q) {
					core.push( plus_minus(P[i],SL[i].vP, !Q), C[i]);
					core.push( plus_minus(P[i],SL[i].T1,  Q), C[i]);
					core.push( plus_minus(P[i],SL[i].vP, !Q), C[i]);//sorry for degenerated triangle
				} else {
					core.push( plus_minus(P[i],SL[i].T1,  Q), C[i]);
					core.push( plus_minus(P[i],SL[i].vP, !Q), C[i]);
					K = !K;
				}
				
				//circular fan
				float den = SL[i].t/SL[i].r*1.0f;
				Point apparent_P = plus_minus(P[i],SL[i].vP, !Q);
				vectors_to_arc( core, P[i], C[i], C[i],
				plus_minus(SL[i].T1, Q), plus_minus(SL[i].T, Q),
				den, SL[i].t, 0.0f, true, &apparent_P);
				
				//succeeding point
				core.push( plus_minus(P[i],SL[i].T,   Q), C[i]);
			} break;
		}	//*/
		
		//inner and outer fade
		for ( int Q=0; Q<=1; Q++)
		{
			switch (SL[i].djoint)
			{
				case LJ_end:
				{
					const Point& cur_cap = i==0? cap1:cap2;
					fade[Q].push( plus_minus(P[i],SL[i].T, Q)-cur_cap, C[i]);
					fade[Q].push( plus_minus(P[i],SL[i].T+SL[i].R, Q)-cur_cap, C[i],true);
				} break;
				
				case LJ_miter:
					fade[Q].push( plus_minus(P[i],SL[i].vP, Q), C[i]);
					fade[Q].push( plus_minus(P[i],SL[i].vP+SL[i].vR, Q), C[i],true);
				break;
				
				case LJ_bevel:
				if ( SL[i].bevel_at_positive == bool(Q)) {
					fade[Q].push( plus_minus(P[i],SL[i].T1, Q), C[i]);
					fade[Q].push( plus_minus(P[i],SL[i].T1+SL[i].R1-SL[i].bR, Q), C[i],true);
					fade[Q].push( plus_minus(P[i],SL[i].T, Q), C[i]);
					fade[Q].push( plus_minus(P[i],SL[i].T+SL[i].R-SL[i+1].bR, Q), C[i],true);
				} else {
					fade[Q].push( plus_minus(P[i],SL[i].vP, Q), C[i]);
					fade[Q].push( plus_minus(P[i],SL[i].vP+SL[i].vR, Q), C[i],true);
				}
				break;
				
				case LJ_round:
				{
				float den = SL[i].t/SL[i].r*1.0f;
				Color C2=C[i]; C2.a=0.0;
				if ( SL[i].bevel_at_positive == bool(Q)) {
					vectors_to_arc( fade[Q], P[i], C[i], C2,
					plus_minus(SL[i].T1, Q), plus_minus(SL[i].T, Q),
					den, SL[i].t, SL[i].t+SL[i].r, false, 0);
				} else {
					//same as miter
					fade[Q].push( plus_minus(P[i],SL[i].vP, Q), C[i]);
					fade[Q].push( plus_minus(P[i],SL[i].vP+SL[i].vR, Q), C[i],true);
				}
				} break;
			}
		} 	//*/
	}
	
	for ( int i=0,k=0; k<=1; i=size_of_P-1, k++)
	{	//caps
		Point& cur_cap = i==0? cap1:cap2;
		if ( cur_cap.non_zero()) //I don't want degeneration
		{
			cap[k].push( Point(P[i])+SL[i].T+SL[i].R-cur_cap, C[i],true);
			cap[k].push( Point(P[i])+SL[i].T+SL[i].R, C[i],true);
			cap[k].push( Point(P[i])+SL[i].T-cur_cap, C[i]);
			cap[k].push( Point(P[i])-SL[i].T-SL[i].R, C[i],true);
			cap[k].push( Point(P[i])-SL[i].T-cur_cap, C[i]);
			cap[k].push( Point(P[i])-SL[i].T-SL[i].R-cur_cap, C[i],true);
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
	
	Point T1,T2,T21,T31;			//these are for calculations in early stage
	
	Point cap1,cap2;				//]these 2 lines store information
	_st_polyline* SL= new _st_polyline[size_of_P];	//]  to be passed to polyline_late()
	
	//early stage
	for ( int i=0; i<size_of_P; i++)
	{
		double r,t;
		//TODO: handle correctly when there are only 2 points
		if ( weight[i]>=0.0 && weight[i]<1.0)
		{
			double f=weight[i]-static_cast<int>(weight[i]);
			C[i].a *=f;
		}
		if ( i < size_of_P-1)
		{
			Point R2,bR;  Point R31,C31; double r31;
			make_T_R_C( Point(P[i]), Point(P[i+1]),  &T2,&R2,&bR, weight[i],opt, &r,&t);
				if ( i==0)
				{
					Point::anchor_outward(R2, P[i+1],P[i+2]);
						T2.follow_signs(R2);
					cap1=bR;
					cap1.opposite(); if ( opt.feather && !opt.no_feather_at_cap)
					cap1*=opt.feathering;
					
					SL[i].djoint=LJ_end;
				}
			
			make_T_R_C( Point(P[i]), Point(P[i+1]), &T31,&R31,&C31, weight[i+1],opt, &r31,0);
			//TODO: (optional) weight compensation when segment is < 1.0px and exactly hori/ vertical
			
			SL[i].T=T2;
			SL[i].R=R2;
			SL[i].bR=bR*0.6;
			SL[i].t=(float)t;
			SL[i].r=(float)r;
			
			SL[i+1].T1=T31;
			SL[i+1].R1=R31;
		}
		
		if ( i==0) {
			//do nothing
		} else if ( i==size_of_P-1) {
			Point R;
			make_T_R_C( P[i-1],P[i],   &T2,&R,&cap2,weight[i],opt,  0,0);
				same_side_of_line( R, SL[i-1].R, P[i-1],P[i]);
					T2.follow_signs(R);
			SL[i].bR=cap2*0.6;
			SL[i].T=T2;	SL[i].R=R;
			SL[i].djoint=LJ_end;
			
			if ( opt.feather && !opt.no_feather_at_cap)
				cap2*=opt.feathering;
		}
		else //2nd to 2nd last point
		{			
			//find the angle between the 2 line segments
			Point ln1,ln2, V;
			ln1 = Point(P[i]) - Point(P[i-1]);
			ln2 = Point(P[i+1]) - Point(P[i]);
			ln1.normalize();
			ln2.normalize();
			Point::dot(ln1,ln2, V);
			double cos_tho=-V.x-V.y;
			
			//make intersection
			Point interP,interR, vP,vR;
			Point::anchor_outward( T1, P[i],P[i+1]);
			Point::anchor_outward( T21, P[i],P[i+1]);
				SL[i].T1.follow_signs(T21);
				SL[i].R1.follow_signs(T21);
			Point::anchor_outward( T2, P[i],P[i-1]);
			Point::anchor_outward( T31, P[i],P[i-1]);
			int result=Point::intersect( Point(P[i-1])+T1, Point(P[i])+T21,
						Point(P[i+1])+T31, Point(P[i])+T2,
						interP);
			if ( result) {
				vP = interP - Point(P[i]);
				vR = vP*(r/t);
				SL[i].vR=vR;
				SL[i].vP=vP;
				// TODO: when line is very thick 
				//  and line segment is very short,
				//  strange things may occur, must be handled
			} else {
				//printf( "intersection failed: angle=%.4f(degree)\n", acos(cos_tho)*180/3.14159);
			}
			
			//make joint
			switch (opt.joint)
			{
				case LJ_miter:
					if ( cos_tho >= cos_cri_angle)
						goto joint_treatment_bevel;
					SL[i].djoint=LJ_miter;
				break;
				
				case LJ_bevel: joint_treatment_bevel:
					SL[i].djoint=LJ_bevel;
				break;
				
				case LJ_round:
					SL[i].djoint=LJ_round;
				break;
			}
			
			//follow inward/outward ness of the previous vector R
			bool ward = Point::anchor_outward_D(SL[i-1].R, P[i],P[i+1]);
			SL[i].bevel_at_positive = ! Point::anchor_outward( SL[i].vP, P[i],P[i+1], !ward);
			Point::anchor_outward( SL[i].R, P[i],P[i-1], !ward);
				SL[i].T.follow_signs( SL[i].R);
			
			if ( cos_tho < -1+0.0001)
			{	//to solve visual bugs 3 and 1.1
				//efficiency: if color and weight is same as previous and next point
				// ,do not generate vertices
				//printf( "i=%d: nearly at 180 degree, result=%d\n", i,result);
				same_side_of_line( SL[i].R, SL[i-1].R, P[i],P[i-1]);
					SL[i].T.follow_signs(SL[i].R);
				SL[i].vP=SL[i].T;
				SL[i].vR=SL[i].R;
			}
		}
		T1=T2;
		T21=T31;
	} //end of for loop
	
	polyline_late( P,C,SL,size_of_P,cap1,cap2);
	
	delete[] SL;
}

#endif
