#ifndef VECTOR_OPERATIONS_H
#define VECTOR_OPERATIONS_H

#include <math.h>

class Point : public Vec2
{
public:
	Point() { clear();}
	Point(const Vec2& P) { set(P.x,P.y);}
	Point(const Point& P) { set(P.x,P.y);}
	Point(double X, double Y) { set(X,Y);}
	
	void clear()			{ x=0.0; y=0.0;}
	void set(double X, double Y)	{ x=X;   y=Y;}
	
	Vec2 vec() {
		Vec2 V;
		V.x = x; V.y = y;
		return V;
	}
	
	//attributes
	double length()
	{
		return sqrt(x*x+y*y);
	}
	double slope()
	{
		return y/x;
	}
	static double signed_area(const Point& P1, const Point& P2, const Point& P3)
	{
		return (P2.x-P1.x)*(P3.y-P1.y) - (P3.x-P1.x)*(P2.y-P1.y);
	}
	
	//vector operators	
	Point operator+(const Point &b) const
	{
		return Point( x+b.x, y+b.y);
	}
	Point operator-() const
	{
		return Point( -x, -y);
	}
	Point operator-(const Point &b) const
	{
		return Point( x-b.x, y-b.y);
	}
	Point operator*(double k) const
	{
		return Point( x*k, y*k);
	}
	
	Point& operator=(const Point& b) {
		x = b.x; y = b.y;
		return *this;
	}
	Point& operator+=(const Point& b)
	{
		x += b.x; y += b.y;
		return *this;
	}
	Point& operator-=(const Point& b)
	{
		x -= b.x; y -= b.y;
		return *this;
	}
	Point& operator*=(const double k)
	{
		x *= k; y *= k;
		return *this;
	}
	
	static void dot( const Point& a, const Point& b, Point& o) //dot product: o = a dot b
	{
		o.x = a.x * b.x;
		o.y = a.y * b.y;
	}
	Point dot_prod( const Point& b) const //return dot product
	{
		return Point( x*b.x, y*b.y);
	}
	
	//self operations
	void opposite()
	{
		x = -x;
		y = -y;
	}
	void opposite_of( const Point& a)
	{
		x = -a.x;
		y = -a.y;
	}
	double normalize()
	{
		double L = length();
		if ( L > vaserend_min_alw)
		{
			x /= L; y /= L;
		}
		return L;
	}
	void perpen() //perpendicular: anti-clockwise 90 degrees
	{
		double y_value=y;
		y=x;
		x=-y_value;
	}
	void follow_signs( const Point& a)
	{
		if ( (x>0) != (a.x>0))	x = -x;
		if ( (y>0) != (a.y>0))	y = -y;
	}
	void follow_magnitude( const Point& a);
	void follow_direction( const Point& a);
	
	//judgements
	static inline bool negligible( double M)
	{
		return -vaserend_min_alw < M && M < vaserend_min_alw;
	}
	bool negligible()
	{
		return Point::negligible(x) && Point::negligible(y);
	}
	bool non_negligible()
	{
		return !negligible();
	}
	bool is_zero()
	{
		return x==0.0 && y==0.0;
	}
	bool non_zero()
	{
		return !is_zero();
	}
	
	//others
	static double distance_squared( const Point& A, const Point& B)
	{
		double dx=A.x-B.x;
		double dy=A.y-B.y;
		return (dx*dx+dy*dy);
	}
	static inline double distance( const Point& A, const Point& B)
	{
		return sqrt( distance_squared(A,B));
	}
	
	static inline bool anchor_outward_D( Point& V, const Point& b, const Point& c)
	{
		return (b.x*V.x - c.x*V.x + b.y*V.y - c.y*V.y) > 0;
	}
	static bool anchor_outward( Point& V, const Point& b, const Point& c, bool reverse=false)
	{ //put the correct outward vector at V, with V placed on b, comparing distances from c
		bool determinant = anchor_outward_D ( V,b,c);
		if ( determinant == (!reverse)) { //when reverse==true, it means inward
			//positive V is the outward vector
			return false;
		} else {
			//negative V is the outward vector
			V.x=-V.x;
			V.y=-V.y;
			return true; //return whether V is changed
		}
	}
	static void anchor_inward( Point& V, const Point& b, const Point& c)
	{
		anchor_outward( V,b,c,true);
	}
	
	static bool opposite_quadrant( const Point& P1, const Point& P2)
	{
		char P1x = P1.x>0? 1:(P1.x<0?-1:0);
		char P1y = P1.y>0? 1:(P1.y<0?-1:0);
		char P2x = P2.x>0? 1:(P2.x<0?-1:0);
		char P2y = P2.y>0? 1:(P2.y<0?-1:0);
		
		if ( P1x!=P2x) {
			if ( P1y!=P2y)
				return true;
			if ( P1y==0 || P2y==0)
				return true;
		}
		if ( P1y!=P2y) {
			if ( P1x==0 || P2x==0)
				return true;
		}
		return false;
	}
	
	static int intersect( const Point& P1, const Point& P2,  //line 1
			const Point& P3, const Point& P4, //line 2
			Point& Pout,			  //the output point
			double* ua_out=0, double* ub_out=0)
	{ //Determine the intersection point of two line segments
		double mua,mub;
		double denom,numera,numerb;

		denom  = (P4.y-P3.y) * (P2.x-P1.x) - (P4.x-P3.x) * (P2.y-P1.y);
		numera = (P4.x-P3.x) * (P1.y-P3.y) - (P4.y-P3.y) * (P1.x-P3.x);
		numerb = (P2.x-P1.x) * (P1.y-P3.y) - (P2.y-P1.y) * (P1.x-P3.x);

		if (	negligible(numera) &&
			negligible(numerb) &&
			negligible(denom)) {
		Pout.x = (P1.x + P2.x) * 0.5;
		Pout.y = (P1.y + P2.y) * 0.5;
		return 2; //meaning the lines coincide
		}

		if ( negligible(denom)) {
			Pout.x = 0;
			Pout.y = 0;
			return 0; //meaning lines are parallel
		}

		mua = numera / denom;
		mub = numerb / denom;
		if ( ua_out) *ua_out = mua;
		if ( ub_out) *ub_out = mub;

		Pout.x = P1.x + mua * (P2.x - P1.x);
		Pout.y = P1.y + mua * (P2.y - P1.y);
		
		bool out1 = mua < 0 || mua > 1;
		bool out2 = mub < 0 || mub > 1;
		
		if ( out1 & out2) {
			return 5; //the intersection lies outside both segments
		} else if ( out1) {
			return 3; //the intersection lies outside segment 1
		} else if ( out2) {
			return 4; //the intersection lies outside segment 2
		} else {
			return 1; //great
		}
	//http://paulbourke.net/geometry/lineline2d/
	}
}; //end of class Point

/* after all,
 * sizeof(Vec2)=16  sizeof(Point)=16
 * Point is not heavier, just more powerful :)
*/

#endif
