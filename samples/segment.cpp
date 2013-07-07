/*config.h is generated by fltk in your system
 * this file is used with fltk 1.3 with gl enabled.
 * compile by: fltk-config --use-gl --compile segment.cpp
 * or something like: g++ -lX11 -lGL 'segment.cpp' -o 'segment'
*/
#include <math.h>
#include <stdio.h>

#include "config.h" //config.h must always be placed before any Fl header
#include <FL/gl.h>
#include <FL/Fl_Box.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Radio_Light_Button.H>

struct Vec2 { double x,y;};
struct Color { float r,g,b,a;};
#define VASE_RENDERER_DEBUG
#include "../include/vase_renderer_draft1_2.cpp"

void test_draw();
#include "test1_base.cpp"

Fl_Window* main_wnd;
Gl_Window* gl_wnd;
Fl_Slider *start_weight, *feathering;
Fl_Button *feather, *no_feather_at_cap, *no_feather_at_core;
Fl_Button *jc_butt, *jc_round, *jc_square, *jc_rect;
Fl_Button *colored, *alphaed, *weighted;

char get_cap_type()
{
	if ( jc_butt->value())
		return LC_butt;
	else if ( jc_round->value())
		return LC_round;
	else if ( jc_square->value())
		return LC_square;
	else if ( jc_rect->value())
		return LC_rect;
	else
		return 0;
}
void enable_glstates()
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	glDisableClientState(GL_EDGE_FLAG_ARRAY);
	glDisableClientState(GL_FOG_COORD_ARRAY);
	glDisableClientState(GL_INDEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}
void disable_glstates()
{
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}
void drag_cb(Fl_Widget* W, void*)
{
	gl_wnd->redraw();
}
void make_form()
{
	const int LD = 606;
	//weight feathering
	start_weight = new Fl_Value_Slider(LD,0,200,20,"spectrum start weight");
	start_weight->type(FL_HOR_SLIDER);
	start_weight->bounds(0.3,30.0);
	start_weight->step(0.3*20.0);
	start_weight->value(0.0);
	start_weight->callback(drag_cb);
	
	feathering = new Fl_Value_Slider(LD,40,200,20,"feathering");
	feathering->type(FL_HOR_SLIDER);
	feathering->bounds(1.0,10.0);
	feathering->callback(drag_cb);
	feathering->value(1.0);
	
	//feather, no_feather_at_cap, no_feather_at_core
	feather		   = new Fl_Light_Button(LD,80,100,15,"feather");
	no_feather_at_cap  = new Fl_Light_Button(LD+50,95,150,15,"no_feather_at_cap");
	no_feather_at_core = new Fl_Light_Button(LD+50,110,150,15,"no_feather_at_core");
	feather		  ->value(1);
	no_feather_at_cap ->value(0);
	no_feather_at_core->value(0);
	feather		  ->callback(drag_cb);
	no_feather_at_cap ->callback(drag_cb);
	no_feather_at_core->callback(drag_cb);
	
	//cap type
	{
	Fl_Group* o = new Fl_Group(LD,160,200,30);
		new Fl_Box(LD,160,80,15,"cap type:");
		jc_butt   = new Fl_Radio_Light_Button(LD+20,175,40,15,"butt");
		jc_round  = new Fl_Radio_Light_Button(LD+60,175,50,15,"round");
		jc_square = new Fl_Radio_Light_Button(LD+110,175,50,15,"square");
		jc_rect   = new Fl_Radio_Light_Button(LD+160,175,40,15,"rect");
	o->end();
	jc_butt   ->value(1);
	jc_butt   ->callback(drag_cb);
	jc_round  ->callback(drag_cb);
	jc_square ->callback(drag_cb);
	jc_rect   ->callback(drag_cb);
	}
	
	//test options
	colored = new Fl_Light_Button(LD,250,60,15,"colored");
	colored->callback(drag_cb);
	colored->value(1);
	alphaed = new Fl_Light_Button(LD+60,250,70,15,"alpha-ed");
	alphaed->callback(drag_cb);
	alphaed->value(1);
	weighted = new Fl_Light_Button(LD+130,250,70,15,"weighted");
	weighted->callback(drag_cb);
}
void test_draw()
{
	enable_glstates();
	
	polyline_opt opt={0};
	
	opt.feather    = feather->value();
	opt.feathering = feathering->value();
	opt.no_feather_at_cap = no_feather_at_cap->value();
	opt.no_feather_at_core = no_feather_at_core->value();
	opt.cap   = get_cap_type();
	
	for ( int i=0; i<20; i++)
	{
		Vec2  P1 = { 5+29.7*i, 187};
		Vec2  P2 = { 35+29.7*i, 8};
		Color C1 = { 0,0,0, 1};
		Color C2 = C1;
		double W1= 0.3*(i+1) + start_weight->value();
		double W2= W1;
		if ( colored->value())
		{
			Color cc1 = { 1.0,0.0,0.5, 1.0};
			C1 = cc1;
			Color cc2 = { 0.5,0.0,1.0, 1.0};
			C2 = cc2;
		}
		if ( alphaed->value())
		{
			C1.a = 0.5f;
			C2.a = 0.5f;
		}
		if ( weighted->value())
		{
			W1 = 0.1;
		}
		if ( opt.cap != LC_butt)
		{
			double end_weight = 0.3*(20) + start_weight->value();
			P1.y -= end_weight*0.5;
			P2.y += end_weight*0.5;
		}
		segment(P1, P2,       //coordinates
			C1, C2,       //colors
			W1, W2,       //weights
			&opt);        //extra options
	}
	
	for ( double ag=0, i=0; ag<2*vaserend_pi-0.1; ag+=vaserend_pi/12, i+=1)
	{
		double r1 = 0.0;
		double r2 = 90.0;
		if ( !weighted->value())
			r1 = 30.0;
		if ( opt.cap != LC_butt)
		{
			double end_weight = 0.3*(12) + start_weight->value();
			r2 -= end_weight*0.5;
		}
		
		double tx2=r2*cos(ag);
		double ty2=r2*sin(ag);
		double tx1=r1*cos(ag);
		double ty1=r1*sin(ag);
		double Ox = 120;
		double Oy = 194+97;
		
		Vec2  P1 = { Ox+tx1,Oy-ty1};
		Vec2  P2 = { Ox+tx2,Oy-ty2};
		Color C1 = { 0,0,0, 1};
		Color C2 = C1;
		double W1= 0.3*(i+1) + start_weight->value();
		double W2= W1;
		if ( colored->value())
		{
			Color cc1 = { 1.0,0.0,0.5, 1.0};
			C1 = cc1;
			Color cc2 = { 0.5,0.0,1.0, 1.0};
			C2 = cc2;
		}
		if ( alphaed->value())
		{
			C1.a = 0.5f;
			C2.a = 0.5f;
		}
		if ( weighted->value())
		{
			W1 = 0.1;
		}
		segment(P1, P2,       //coordinates
			C1, C2,       //colors
			W1, W2,       //weights
			&opt);        //extra options
	}
	
	disable_glstates();
}

int main(int argc, char **argv)
{
	vaserend_actual_PPI = 111.94;
	//
	main_wnd = new Fl_Window( 606+200,194*2,"Vase Renderer - segment() example - fltk/opengl");
		make_form(); //initialize
		gl_wnd = new Gl_Window( 0,0,606,194*2);  gl_wnd->end(); //create gl window
	main_wnd->end();
	main_wnd->show();
	main_wnd->redraw();
	
	return Fl::run();
}