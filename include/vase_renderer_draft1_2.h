#ifndef VASE_RENDERER_DRAFT1_2_H
#define VASE_RENDERER_DRAFT1_2_H
/*Vase Renderer first draft, version 0.2 
 *(VaseR draft1_2)*/

/* You should provide these structs to VaseR before any vase_renderer_* include
struct Vec2 { double x,y;};
struct Color { float r,g,b,a;};
* or
typedef your_vec2 Vec2;
typedef your_color Color;
*/

//Vaserend global variables
const double vaserend_min_alw=0.00000000001; //smallest value not regarded as zero
const double vaserend_pi=3.141592653589793;

double vaserend_actual_PPI = 96.0;
const double vaserend_standard_PPI = 111.94; //the PPI I used for calibration

struct polyline_opt
{	//set the whole structure to 0 will give default options
	char joint;
		#define LJ_miter 0
		#define LJ_bevel 1
		#define LJ_round 2
	char cap;
		#define LC_butt   0
		#define LC_round  1
		#define LC_square 2
		#define LC_rect   3 //unique to vase renderer
	bool feather;
		double feathering;
		bool no_feather_at_cap;
		bool no_feather_at_core;
	
	//bool uniform_color;
	//bool uniform_weight;
};
void polyline( const Vec2*, const Color*, const double*, int, const polyline_opt*);
void segment(  const Vec2& P1, const Vec2& P2, const Color& C1, const Color& C2,
		double W1, double W2, const polyline_opt* options)
{
	Vec2   AP[2];
	Color  AC[2];
	double AW[2];
		AP[0] = P1; AC[0] = C1; AW[0] = W1;
		AP[1] = P2; AC[1] = C2; AW[1] = W2;
	polyline( AP, AC, AW, 2, options);
}

#endif
