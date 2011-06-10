#include "config.h"
#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>

#include "vase_renderer_draft1_2.h"

class Gl_Window : public Fl_Gl_Window
{
	//methods
	void init();
	void draw();
	int handle(int);
	void left_mouse_button_down();
	void left_mouse_button_up();
	void leftclick_drag();
	
	//variables
	short curx,cury, lastx,lasty;
	short cur_drag; //index of point which is being draged currently
			// -1 means none
	int tsize;
	Vec2* target;

	public:
	Gl_Window(int x,int y,int w,int h,const char *l=0)
		: Fl_Gl_Window(x,y,w,h,l) { init();}
	Gl_Window(int w,int h,const char *l=0)
		: Fl_Gl_Window(w,h,l) { init();}
	void set_drag_target( Vec2* target, int size_of_target)
		{ this->target=target; this->tsize=size_of_target;}
};
void Gl_Window::init()
{
	curx=0; cury=0; lastx=0; lasty=0;
	cur_drag=-1;
	tsize=0;
	target=0;
}
void Gl_Window::draw()
{
	if (!valid())
	{
		glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho( 0,w(),h(),0,0.0f,100.0f);
			glClearColor( 1.0,1.0,1.0,1.0f);
			glClearDepth( 0.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	test_draw();
}
void Gl_Window::left_mouse_button_down()
{
	for ( int i=0; i<tsize; i++)
	{
		const short ALW=10;
		short dx=curx-target[i].x;
		short dy=cury-target[i].y;
		if ( -ALW<dx && dx<ALW)
		if ( -ALW<dy && dy<ALW)
		{
			cur_drag=i;
			return;
		}
	}
	cur_drag=-1;
};
void Gl_Window::left_mouse_button_up()
{
};
void Gl_Window::leftclick_drag()
{
	if ( target)
	if ( cur_drag >=0 && cur_drag< tsize)
	{
		target[cur_drag].x=curx;
		target[cur_drag].y=cury;
	}
	this->redraw();
};
int Gl_Window::handle(int event)
{
	switch(event)
	{
		case FL_PUSH:
		case FL_RELEASE:
		case FL_DRAG:
		case FL_MOVE:
			curx = Fl::event_x();
			cury = Fl::event_y();
			switch(event)
			{
				case FL_PUSH:
					if ( Fl::event_button() == FL_LEFT_MOUSE)
						left_mouse_button_down();
				case FL_RELEASE:
					if ( Fl::event_button() == FL_LEFT_MOUSE)
						left_mouse_button_up();
				case FL_DRAG:
					if ( (Fl::event_state()&FL_BUTTON1)==FL_BUTTON1)
						leftclick_drag();
				case FL_MOVE:
					;
			}
			lastx=curx;
			lasty=cury;
		return 1;
		
		default:
		return Fl_Window::handle(event);
	} //end of switch(event)
};
