#include "../config.h"
#include <math.h>
#include <stdio.h>
#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>

void test_draw();

class Gl_Window : public Fl_Gl_Window
{
	void draw();
	int handle(int);

	public:
	Gl_Window(int x,int y,int w,int h,const char *l=0)
		: Fl_Gl_Window(x,y,w,h,l) {}
	Gl_Window(int w,int h,const char *l=0)
		: Fl_Gl_Window(w,h,l) {}
};
int Gl_Window::handle(int e)
{
	return Fl_Gl_Window::handle(e);
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

void test_draw()
{
	float para_vertex[]=
	{
		0,400,
		0,0,
		400,400,
		400,0
	};
	float para_color[]=
	{
		1,1,1,
		1,1,1,
		0.5,0,1,
		1,1,1
	};
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	glVertexPointer(2, GL_FLOAT, 0, para_vertex);
	glColorPointer(3, GL_FLOAT, 0, para_color);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

int main(int argc, char **argv)
{
	Gl_Window gl_wnd( 400,400,"test12 (fltk-opengl)");
	gl_wnd.show();
	gl_wnd.redraw();
	return Fl::run();
}
