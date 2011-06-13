#ifndef VASE_RENDERER_DRAFT1_2_H
#define VASE_RENDERER_DRAFT1_2_H

/* you should provide these structs to Vaserend before any vase_renderer include
struct Vec2 { double x,y;};
struct Color { float r,g,b,a;};
* or
typedef your_vec2 Vec2;
typedef your_color Color;
*/

//Global variables
const double vaserend_min_alw=0.00000000001; //smallest value not regarded as zero
const double vaserend_pi=3.141592653589793;

//Vaserend global variables
double vaserend_actual_PPI = 96.0;
const double vaserend_standard_PPI = 111.94; //the PPI I used for calibration

struct polyline_opt
{	//set the whole structure to 0 will give default options
	char joint;
		#define LJ_miter 0
		#define LJ_bevel 1
		#define LJ_round 2
	char cap;
		#define LC_inwardcap 0
	bool feather;
		double feathering;
		bool no_feather_at_cap;
		bool no_feather_at_core;
	
	//bool ignor_first_segment;
	//bool ignor_last_segment;
	//bool uniform_color;
	//bool uniform_weight;
};

void polyline( Vec2* P, Color* C, double* weight, int size_of_P, polyline_opt* options);

#endif
