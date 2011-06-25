#ifndef VERTEX_ARRAY_HOLDER_H
#define VERTEX_ARRAY_HOLDER_H

class vertex_array_holder
{
	int count; //counter
	int glmode; //drawing mode in opengl
	bool jumping;
public:
	const static int MAX_VERT=128;
	float vert[MAX_VERT*2]; //because it holds 2d vectors
	float color[MAX_VERT*4]; //RGBA
	
	vertex_array_holder()
	{
		count = 0;
		glmode = GL_TRIANGLES;
		jumping = false;
	}
	
	void set_gl_draw_mode( int gl_draw_mode)
	{
		glmode = gl_draw_mode;
	}
	
	int get_count()
	{
		return count;
	}
	
	void move( int a, int b) //move b into a
	{
		vert[a*2]   = vert[b*2];
		vert[a*2+1] = vert[b*2+1];
		
		color[a*4]  = color[b*4];
		color[a*4+1]= color[b*4+1];
		color[a*4+2]= color[b*4+2];
		color[a*4+3]= color[b*4+3];
	}
	void replace( int a, Point P, Color C)
	{
		vert[a*2]   = P.x;
		vert[a*2+1] = P.y;
		
		color[a*4]  = C.r;
		color[a*4+1]= C.g;
		color[a*4+2]= C.b;
		color[a*4+3]= C.a;
	}
	
	int push( const Point& P, const Color& cc, bool transparent=false)
	{
		//if ( P.x==0 && P.y==0)
			//printf("who pushed P(0,0)?\n");
		
		int& i = count;
		int cur = count;
		
		vert[i*2]  = P.x;
		vert[i*2+1]= P.y;
		
		color[i*4]  = cc.r;
		color[i*4+1]= cc.g;
		color[i*4+2]= cc.b;
		if ( transparent==true)
			color[i*4+3]= 0.0f;
		else
			color[i*4+3]= cc.a;
		
		i++;
		if ( i == MAX_VERT) //when the internal array is full
		{			//draw and flush
			draw();
			switch( glmode)
			{
				case GL_POINTS:
					//do nothing
				break;
				
				case GL_LINES:
					if ( i%2 == 0) {
						//do nothing
					} else {
						goto copy_the_last_point;
					}
				break;
				
				case GL_TRIANGLES:
					if ( i%3 == 0) {
						//do nothing
					} else if ( i%3 == 1) {
						goto copy_the_last_point;
					} else {
						goto copy_the_last_2_points;
					}
				break;
				
				case GL_LINE_STRIP: case GL_LINE_LOOP:
				//for line loop it is not correct
				copy_the_last_point:
					move(0,MAX_VERT-1);
					cur=0;
					i=1;
				break;
				
				case GL_TRIANGLE_STRIP:
				copy_the_last_2_points:
					move(0,MAX_VERT-2);
					move(1,MAX_VERT-1);
					cur=1;
					i=2;
				break;
				
				case GL_TRIANGLE_FAN:
					//retain the first point,
					// and copy the last point
					move(1,MAX_VERT-1);
					cur=1;
					i=2;
				break;
				
				case GL_QUAD_STRIP:
				case GL_QUADS:
				case GL_POLYGON:
				//let it be and I cannot help
					i=0;
				break;
			}
			if ( i == MAX_VERT) //as a double check
				i=0;
		}
		
		if ( jumping)
		{
			jumping=false;
			repeat_last_push();
		}
		
		return cur;
	}
	
	Point get(int i)
	{
		Point P;
		P.x = vert[i*2];
		P.y = vert[i*2+1];
		return P;
	}
	Color get_color(int b)
	{
		Color C;
		C.r = color[b*4];
		C.g = color[b*4+1];
		C.b = color[b*4+2];
		C.a = color[b*4+3];
		return C;
	}
	Point get_relative_end(int di=-1)
	{	//di=-1 is the last one
		int i = count+di;
		if ( i<0) i=0;
		if ( i>=MAX_VERT) i=MAX_VERT-1;
		return get(i);
	}
	void repeat_last_push()
	{
		Point P;
		Color cc;
		
		int i = count-1;
		
		P.x = vert[i*2];
		P.y = vert[i*2+1];
		
		cc.r = color[i*4];
		cc.g = color[i*4+1];
		cc.b = color[i*4+2];
		cc.a = color[i*4+3];
		
		push(P,cc);
	}
	void jump() //to make a jump in triangle strip by degenerated triangles
	{
		repeat_last_push();
		jumping=true;
	}
	
	void draw() //the only place to call gl functions
	{
		if ( count > 0) //save some effort
		{
			glVertexPointer(2, GL_FLOAT, 0, vert);
			glColorPointer (4, GL_FLOAT, 0, color);
			glDrawArrays (glmode, 0, count);
		}
	}
};

#endif
