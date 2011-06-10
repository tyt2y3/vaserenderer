#include "config.h" //config.h must always be placed before any Fl header
#include <FL/gl.h>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Radio_Light_Button.H>

void test_draw();
#include "test1_base.cpp"
#include "vase_renderer_draft1_2.cpp"

#include <math.h>
#include <stdio.h>

const int buf_size=10;

Point AP[buf_size]; int size_of_AP=3;
Color AC[buf_size];
double Aw[buf_size];

Fl_Window* main_wnd;
Gl_Window* gl_wnd;
Fl_Slider* weight;
Fl_Slider* feathering;
Fl_Button *feather, *no_feather_at_cap, *no_feather_at_core;

void line_update()
{
	for ( int i=0; i<buf_size; i++)
	{
		Color cc = {0,0,0,1};
		AC[i] = cc;
		Aw[i] = weight->value();
	}
}
void line_init()
{
	AP[0].x=200; AP[0].y=100;
	AP[1].x=100; AP[1].y=200;
	AP[2].x=300; AP[2].y=200;
	line_update();
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
	
	char wei_label[30]={0};
	sprintf(wei_label,"weight=%.2f",weight->value());
	weight->copy_label(wei_label);
	
	char fea_label[30]={0};
	sprintf(fea_label,"feathering=%.2f",feathering->value());
	feathering->copy_label(fea_label);
}
void make_form()
{
	//weight and feathering
	weight = new Fl_Slider(FL_HOR_SLIDER,400,0,200,20,"weight");
	feathering = new Fl_Slider(FL_HOR_SLIDER,400,40,200,20,"feathering");
	weight->bounds(0.01,20.0);
	feathering->bounds(1.0,10.0);
	weight->callback(drag_cb);
	feathering->callback(drag_cb);
	
	weight->value(6.0);
	feathering->value(1.0);
	
	//feather, no_feather_at_cap, no_feather_at_core
	feather		   = new Fl_Light_Button(400,80,100,15,"feather");
	no_feather_at_cap  = new Fl_Light_Button(450,95,150,15,"no_feather_at_cap");
	no_feather_at_core = new Fl_Light_Button(450,110,150,15,"no_feather_at_core");
	feather		  ->value(1);
	no_feather_at_cap ->value(0);
	no_feather_at_core->value(0);
	feather		  ->callback(drag_cb);
	no_feather_at_cap ->callback(drag_cb);
	no_feather_at_core->callback(drag_cb);
}
void test_draw()
{
	enable_glstates();
	
	line_update();
	
	polyline_opt opt={0};
	
	opt.feather    = feather->value();
	opt.feathering = feathering->value();
	opt.no_feather_at_cap = no_feather_at_cap->value();
	opt.no_feather_at_core = no_feather_at_core->value();
	
	polyline( AP, AC, Aw, size_of_AP, &opt);
	
	disable_glstates();
}

int main(int argc, char **argv)
{
	main_wnd = new Fl_Window( 600,300,"test1 (fltk-opengl)");
		make_form(); //initialize
		gl_wnd = new Gl_Window( 0,0,400,300);  gl_wnd->end(); //create gl window
		line_init();  gl_wnd->set_drag_target( AP, size_of_AP); //set up drag
	main_wnd->end();
	main_wnd->show();
	main_wnd->redraw();
	
	return Fl::run();
}
