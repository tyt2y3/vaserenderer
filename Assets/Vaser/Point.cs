namespace Vaser
{
	public struct Point
	{
		public float x { get; set; }
		public float y { get; set; }

		public Point(Point P) { x = P.x; y = P.y; }
		public Point(float X, float Y) { x = X; y = Y; }

		// attributes
		public float Length()
		{
			return (float) System.Math.Sqrt(x*x+y*y);
		}

		public static float SignedArea(Point P1, Point P2, Point P3)
		{
			return (P2.x-P1.x)*(P3.y-P1.y) - (P3.x-P1.x)*(P2.y-P1.y);
		}

		// operators	
		public static Point operator + (Point a, Point b)
		{
			return new Point(a.x + b.x, a.y + b.y);
		}
		public static Point operator - (Point a)
		{
			return new Point(-a.x, -a.y);
		}
		public static Point operator - (Point a, Point b)
		{
			return new Point(a.x-b.x, a.y-b.y);
		}
		public static Point operator * (Point a, float k)
		{
			return new Point(a.x*k, a.y*k);
		}

		/*Point& operator=(const Point& b) {
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
		Point& operator*=(const float k)
		{
			x *= k; y *= k;
			return *this;
		}
		Point& operator/=(const float k)
		{
			x /= k; y /= k;
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
		
		// self operations
		void opposite()
		{
			x = -x;
			y = -y;
		}
		void opposite_of( const Point& a)
		{
			x = -a.x;
			y = -a.y;
		} */
		public float normalize()
		{
			float L = Length();
			if ( L > 0.0000001)
			{
				x /= L; y /= L;
			}
			return L;
		}
		/*void perpen() //perpendicular: anti-clockwise 90 degrees
		{
			float y_value=y;
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
		static inline bool negligible( float M)
		{
			return -vaser_min_alw < M && M < vaser_min_alw;
		}
		bool negligible() const
		{
			return Point::negligible(x) && Point::negligible(y);
		}
		bool non_negligible() const
		{
			return !negligible();
		}
		bool is_zero() const
		{
			return x==0.0 && y==0.0;
		}
		bool non_zero() const
		{
			return !is_zero();
		}
		static bool intersecting( const Point& A, const Point& B,
			const Point& C, const Point& D)
		{	//return true if AB intersects CD
			return signed_area(A,B,C)>0 != signed_area(A,B,D)>0;
		}
		
		//operations require 2 input points
		static float distance_squared( const Point& A, const Point& B)
		{
			float dx=A.x-B.x;
			float dy=A.y-B.y;
			return (dx*dx+dy*dy);
		}
		static inline float distance( const Point& A, const Point& B)
		{
			return sqrt( distance_squared(A,B));
		}
		static Point midpoint( const Point& A, const Point& B)
		{
			return (A+B)*0.5;
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
		
		//operations of 3 points
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
		
		//operations of 4 points
		static char intersect( const Point& P1, const Point& P2,  //line 1
				const Point& P3, const Point& P4, //line 2
				Point& Pout,			  //the output point
				float* ua_out=0, float* ub_out=0)
		{ //Determine the intersection point of two line segments
			float mua,mub;
			float denom,numera,numerb;

			denom  = (P4.y-P3.y) * (P2.x-P1.x) - (P4.x-P3.x) * (P2.y-P1.y);
			numera = (P4.x-P3.x) * (P1.y-P3.y) - (P4.y-P3.y) * (P1.x-P3.x);
			numerb = (P2.x-P1.x) * (P1.y-P3.y) - (P2.y-P1.y) * (P1.x-P3.x);

			if( negligible(numera) &&
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
		} */
	}
}