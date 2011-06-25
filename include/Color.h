#ifndef VASE_RENDERER_COLOR_H
#define VASE_RENDERER_COLOR_H

#include <math.h>

float& Color_get( Color& C, int index)
{
	switch (index)
	{
		case 0: return C.r;
		case 1: return C.g;
		case 2: return C.b;
		default:return C.r;
	}
}

Color Color_between( const Color& A, const Color& B, float t=0.5f)
{
	float kt = 1.0f - t;
	Color C = { A.r *kt + B.r *t,
		A.g *kt + B.g *t,
		A.b *kt + B.b *t,
		A.a *kt + B.a *t
	};
	return C;
}

void sRGBtolinear( Color& C, bool exact=false)
{	//de-Gamma 2.2
	//from: http://www.xsi-blog.com/archives/133
	if (exact)
	{
		for ( int i=0; i<3; i++)
		{
			float& cc = Color_get( C,i);
			
			if ( cc > 0.04045)
				cc = pow( (cc+0.055)/1.055, 2.4);
			else
				cc /= 12.92;
		}
	}
	else
	{	//approximate
		for ( int i=0; i<3; i++)
		{
			float& cc = Color_get( C,i);
			cc = pow(cc,2.2);
		}
	}
}
void lineartosRGB( Color& C, bool exact=false)
{	//Gamma 2.2
	if (exact)
	{
		for ( int i=0; i<3; i++)
		{
			float& cc = Color_get( C,i);
			if ( cc > 0.0031308)
				cc = 1.055 * pow(cc,1.0/2.4) - 0.055;
			else
				cc *= 12.92;
		}
	}
	else
	{	//approximate
		for ( int i=0; i<3; i++)
		{
			float& cc = Color_get( C,i);
			cc = pow(cc,1.0/2.2);
		}
	}
}

static inline float MAX( float r,float g,float b)
{
	return r>g? (g>b?r:(r>b?r:b)) : (g>b?g:b);
}
static inline float MIN( float r, float g, float b)
{
	return -MAX( -r,-g,-b);
}
void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v )
{	//from: http://www.cs.rit.edu/~ncs/color/t_convert.html
	// r,g,b values are from 0 to 1
	// h = [0,360], s = [0,1], v = [0,1]
	//		if s == 0, then h = -1 (undefined)
	float min, max, delta;
	min = MIN( r, g, b );
	max = MAX( r, g, b );
	*v = max;				// v
	delta = max - min;
	if( max != 0 )
		*s = delta / max;		// s
	else {
		// r = g = b = 0		// s = 0, v is undefined
		*s = 0;
		*h = -1;
		return;
	}
	if( r == max )
		*h = ( g - b ) / delta;		// between yellow & magenta
	else if( g == max )
		*h = 2 + ( b - r ) / delta;	// between cyan & yellow
	else
		*h = 4 + ( r - g ) / delta;	// between magenta & cyan
	*h *= 60;				// degrees
	if( *h < 0 )
		*h += 360;
}
void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
	int i;
	float f, p, q, t;
	if( s == 0 ) {
		// achromatic (grey)
		*r = *g = *b = v;
		return;
	}
	h /= 60;			// sector 0 to 5
	i = floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );
	switch( i ){
		case 0:
			*r = v; *g = t; *b = p;		break;
		case 1:
			*r = q; *g = v;	*b = p;		break;
		case 2:
			*r = p;	*g = v;	*b = t;		break;
		case 3:
			*r = p;	*g = q;	*b = v;		break;
		case 4:
			*r = t;	*g = p;	*b = v;		break;
		default:		// case 5:
			*r = v;	*g = p;	*b = q;		break;
	}
}

#endif
