#ifndef VASE_RENDERER_DRAFT1_2_H
#define VASE_RENDERER_DRAFT1_2_H
/*Vase Renderer
 * first draft, version 0.2 (draft1_2)*/

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
	bool feather;
		double feathering;
		bool no_feather_at_cap;
		bool no_feather_at_core;
	
	//bool uniform_color;
	//bool uniform_weight;
};
void polyline( Vec2*, Color*, double*, int, polyline_opt*);

#endif
