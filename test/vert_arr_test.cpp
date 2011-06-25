/*sample usage and test of vertex_array_holder
*/
#include "../samples/config.h"
#include <math.h>
#include <stdio.h>
#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>

struct Vec2 { double x,y;};
struct Color { float r,g,b,a;};
#include "../include/vase_renderer_draft1_2.cpp"

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
		
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	test_draw();
}
void test_draw()
{
	Color cc[2];
	cc[0].r=1.0f; cc[0].g=0.0f; cc[0].b=0.5f; cc[0].a=1.0f;
	cc[1].r=0.5f; cc[1].g=0.0f; cc[1].b=1.0f; cc[1].a=1.0f;
	
	
	vertex_array_holder strip;
	strip.set_gl_draw_mode(GL_TRIANGLE_STRIP);
	for ( int i=0; i<10; i++)
	{
		strip.push( Point(10+i*30,10),cc[i%2==0?0:1]);
		strip.push( Point(10+i*30,40),cc[i%2==0?0:1]);
	}
	
	vertex_array_holder ln_strip;
	ln_strip.set_gl_draw_mode(GL_LINE_STRIP);
	for ( int i=0; i<10; i++)
	{
		ln_strip.push( Point(10+i*30,50),cc[i%2==0?0:1]);
	}
	
	vertex_array_holder tri;
	tri.set_gl_draw_mode(GL_TRIANGLES);
	for ( int i=0; i<10; i++)
	{
		tri.push( Point(10+i*30,80),cc[i%2==0?0:1]);
		tri.push( Point(25+i*30,60),cc[i%2==0?0:1]);
		tri.push( Point(40+i*30,80),cc[i%2==0?0:1]);
	}
	vah_knife_cut( tri, Point(0,60),
			Point(400,100),
			Point(0,90) );
	glLineWidth(1.0);
	glBegin(GL_LINES);
		glColor3f(0,0,0);
		glVertex2f(0,60);
		glVertex2f(400,100);
	glEnd();
	
	vertex_array_holder fan;
	fan.set_gl_draw_mode(GL_TRIANGLE_FAN);
	fan.push( Point(100,140),cc[1]);
	for ( int i=0; i<10; i++)
	{
		double x=50*cos(i*2*3.14159/9);
		double y=50*sin(i*2*3.14159/9);
		fan.push( Point(100+x,140-y),cc[i%2==0?0:1]);
	}
	
	strip.draw();
	ln_strip.draw();
	tri.draw();
	fan.draw();
	
	gl_font( FL_HELVETICA, 8); glColor3f(0,0,0);
	gl_draw( "GL_TRIANGLE_STRIP",285,30);
	gl_draw( "GL_LINE_STRIP",285,55);
	gl_draw( "GL_TRIANGLES",310,75);
	gl_draw( "GL_TRIANGLE_FAN",155,145);
	gl_draw( "knife",350,95);
}


int main(int argc, char **argv)
{
	Gl_Window gl_wnd( 400,200,"vertex array holder test (fltk-opengl)");
	gl_wnd.show();
	gl_wnd.redraw();
	return Fl::run();
}
