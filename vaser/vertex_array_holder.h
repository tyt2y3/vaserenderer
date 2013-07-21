#ifndef VASER_VERTEX_ARRAY_HOLDER_H
#define VASER_VERTEX_ARRAY_HOLDER_H

class vertex_array_holder
{
public:
	int count; //counter
	int glmode; //drawing mode in opengl
	bool jumping;
	std::vector<float> vert; //because it holds 2d vectors
	std::vector<float> color; //RGBA

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
	
	void clear()
	{
		count = 0;
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
	
	/* int draw_and_flush()
	{
		int& i = count;
		draw();
		switch( glmode)
		{
			case GL_POINTS:
				i=0;
			break;
			
			case GL_LINES:
				if ( i%2 == 0) {
					i=0;
				} else {
					goto copy_the_last_point;
				}
			break;
			
			case GL_TRIANGLES:
				if ( i%3 == 0) {
					i=0;
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
				i=1;
			break;
			
			case GL_TRIANGLE_STRIP:
			copy_the_last_2_points:
				move(0,MAX_VERT-2);
				move(1,MAX_VERT-1);
				i=2;
			break;
			
			case GL_TRIANGLE_FAN:
				//retain the first point,
				// and copy the last point
				move(1,MAX_VERT-1);
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
	}*/
	
	int push( const Point& P, const Color& cc, bool trans=false)
	{
		int cur = count;
		vert.push_back(P.x);
		vert.push_back(P.y);
		color.push_back(cc.r);
		color.push_back(cc.g);
		color.push_back(cc.b);
		color.push_back(trans?0.0f:cc.a);

		count++;
		if ( jumping)
		{
			jumping=false;
			repeat_last_push();
		}
		return cur;
	}
	
	void push3( const Point& P1, const Point& P2, const Point& P3,
			const Color& C1, const Color& C2, const Color& C3,
			bool trans1=0, bool trans2=0, bool trans3=0)
	{
		push( P1,C1,trans1);
		push( P2,C2,trans2);
		push( P3,C3,trans3);
	}
	
	void push( const vertex_array_holder& hold)
	{
		if ( glmode == hold.glmode)
		{
			count += hold.count;
			vert.insert(vert.end(), hold.vert.begin(), hold.vert.end());
			color.insert(color.end(), hold.color.begin(), hold.color.end());
		}
		else if ( glmode == GL_TRIANGLES &&
			hold.glmode == GL_TRIANGLE_STRIP)
		{		
			int& a = count;
			for (int b=2; b < hold.count; b++)
			{
				for ( int k=0; k<3; k++,a++)
				{
					int B = b-2 + k;
					vert.push_back(hold.vert[B*2]);
					vert.push_back(hold.vert[B*2+1]);
					color.push_back(hold.color[B*4]);
					color.push_back(hold.color[B*4+1]);
					color.push_back(hold.color[B*4+2]);
					color.push_back(hold.color[B*4+3]);
				}
			}
		}
		else
		{
			DEBUG( "vertex_array_holder:push: unknown type\n");
		}
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
		if ( i>=count) i=count-1;
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
		if ( glmode == GL_TRIANGLE_STRIP)
		{
			repeat_last_push();
			jumping=true;
		}
	}
	void draw()
	{
		backend::vah_draw(*this);
	}
	void draw_triangles()
	{
		Color col={1 , 0, 0, 0.5};
		if ( glmode == GL_TRIANGLES)
		{
			for ( int i=0; i<count; i++)
			{
				Point P[4];
				P[0] = get(i); i++;
				P[1] = get(i); i++;
				P[2] = get(i);
				P[3] = P[0];
				polyline((Vec2*)P,col,1.0,4,0);
			}
		}
		else if ( glmode == GL_TRIANGLE_STRIP)
		{
			for ( int i=2; i<count; i++)
			{
				Point P[3];
				P[0] = get(i-2);
				P[1] = get(i);
				P[2] = get(i-1);
				polyline((Vec2*)P,col,1.0,3,0);
			}
		}
	}
	void swap(vertex_array_holder& B)
	{
		int hold_count=count;
		int hold_glmode=glmode;
		bool hold_jumping=jumping;
		count = B.count;
		glmode = B.glmode;
		jumping = B.jumping;
		B.count = hold_count;
		B.glmode = hold_glmode;
		B.jumping = hold_jumping;
		vert.swap(B.vert);
		color.swap(B.color);
	}
};

#endif
